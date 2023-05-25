
# macro to configure the use of local or system version of a package
# Uses:
#	HAVE_LOCAL_${name} - is local version available?
#	${name}_LOCAL_LIBRARIES - libraries of the local version
#	${name}_LOCAL_INCLUDE_DIRS - include directories of the local version
#	HAVE_SYSTEM_${name} - is system version available?
#	${name}_SYSTEM_LIBRARIES - libraries of the system version
#	${name}_SYSTEM_INCLUDE_DIRS - include directories of the system version
# Generates:
#	WITH_LOCAL_${name} - use the local version of the package (only when local is available)
#	WITH_${name} - use this package
#	${name}_LIBRARIES - libraries
#	${name}_INCLUDE_DIRS - include directories
macro( CONFIGURE_WITH_LOCAL_OR_SYSTEM name )
	# 对现有的变量进行清理和重置
	unset( ${name}_LIBRARIES CACHE )
	unset( ${name}_INCLUDE_DIRS CACHE )

	# 若定义了 HAVE_LOCAL_${name}
	# 疑问: 整个系统中谁负责定义这个 HAVE_LOCAL_${name}
	if( HAVE_LOCAL_${name} )
		# 那么定义对应的 WITH_LOCAL_${name} 变量
		set( WITH_LOCAL_${name} ON CACHE BOOL "use local version of ${name}" )
	else()
		# 否则就直接取消对 WITH_LOCAL_${name} 的定义
		unset( WITH_LOCAL_${name} CACHE )
	endif()

	# 若上面成功定义了 WITH_LOCAL_${name}
	if( WITH_LOCAL_${name} )
		# 提示正在配置这个本地库相关的变量
		message( STATUS "Configuring for local ${name}" )

		# 将 ${name}_LIBRARIES 的设置为 ${name}_LOCAL_LIBRARIES}
		# 这里的 ${name}_LOCAL_LIBRARIES 是在各个 3rdparty/*/CMakeLists.txt 中定义的
		set( ${name}_LIBRARIES ${${name}_LOCAL_LIBRARIES} )

		# 将 ${name}_INCLUDE_DIRS 的设置为 ${name}_LOCAL_INCLUDE_DIRS
		# 这里的 ${name}_LOCAL_INCLUDE_DIRS 是在各个 3rdparty/*/CMakeLists.txt 中定义的
		set( ${name}_INCLUDE_DIRS ${${name}_LOCAL_INCLUDE_DIRS} )

		# 提示已经完成这个本地库的变量设置
		message( STATUS "Configuring for local ${name} - done" )
	elseif( HAVE_SYSTEM_${name} )
		# 否则, 如果定义了 HAVE_SYSTEM_${name}

		# 提示正在配置这个系统库相关的变量
		message( STATUS "Configuring for system ${name}" )

		# 将 ${name}_LIBRARIES 的设置为 ${name}_SYSTEM_LIBRARIES}
		# 这里的 ${name}_SYSTEM_LIBRARIES 是在各个 3rdparty/*/CMakeLists.txt 中定义的
		set( ${name}_LIBRARIES ${${name}_SYSTEM_LIBRARIES} )

		# 将 ${name}_INCLUDE_DIRS 的设置为 ${name}_SYSTEM_INCLUDE_DIRS
		# 这里的 ${name}_SYSTEM_INCLUDE_DIRS 是在各个 3rdparty/*/CMakeLists.txt 中定义的
		set( ${name}_INCLUDE_DIRS ${${name}_SYSTEM_INCLUDE_DIRS} )

		# 提示已经完成这个系统库的变量设置
		message( STATUS "Configuring for system ${name} - done" )
	endif()

	# 只要存在这个名字为 ${name} 的本地库或者系统库中的任何一个
	if( WITH_LOCAL_${name} OR HAVE_SYSTEM_${name} )
		# 那么设置上 WITH_${name} 这个布尔变量
		set( WITH_${name} ON CACHE BOOL "use ${name}" )
	else()
		# 如果本地库和系统库都不存在, 那么取消对 WITH_${name} 的定义
		unset( WITH_${name} CACHE )
	endif()

	# 将 ${name}_LIBRARIES 的值保存到缓存中, 指定的类型为路径选择器. 主要是给 CMake GUI 使用
	set( ${name}_LIBRARIES ${${name}_LIBRARIES} CACHE PATH "${name} libraries" )

	# 将 ${name}_INCLUDE_DIRS 的值保存到缓存中, 指定的类型为路径选择器. 主要是给 CMake GUI 使用
	set( ${name}_INCLUDE_DIRS ${${name}_INCLUDE_DIRS} CACHE PATH "${name} include directories" )

	# 将 ${name}_LIBRARIES 提升为高级变量
	# 高级变量指的是那些在 CMake GUI 中，只有当“显示高级选项”被打开时才会被显示的变量
	mark_as_advanced( ${name}_LIBRARIES )

	# 将 ${name}_INCLUDE_DIRS 提升为高级变量
	# 高级变量指的是那些在 CMake GUI 中，只有当“显示高级选项”被打开时才会被显示的变量
	mark_as_advanced( ${name}_INCLUDE_DIRS )
endmacro( CONFIGURE_WITH_LOCAL_OR_SYSTEM )


add_subdirectory( httplib )
add_subdirectory( json )
add_subdirectory( libconfig )
add_subdirectory( mysql )
add_subdirectory( pcre )
add_subdirectory( rapidyaml )
add_subdirectory( yaml-cpp )
add_subdirectory( zlib )
add_subdirectory( breakpad )
add_subdirectory( cryptopp )

include(CPM)

#
# 使用 CPM 模块引入 opencc 第三方库
#

CPMAddPackage(
	NAME opencc
	OPTIONS		"BUILD_SHARED_LIBS OFF"
	SOURCE_DIR	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/3rdparty/opencc"
)

# 若 opencc 引入成功, 则进行一系列的额外配置操作
if(opencc_ADDED)
	# 确保 ${opencc_SOURCE_DIR}/src 目录中存在 opencc.h 文件, 
	# 将目录的路径保存到 opencc_INCLUDE_DIRS 变量中 (声明为 REQUIRED 如果失败则终止)
	# 后面 common 等项目构建的时候需要将 opencc_INCLUDE_DIRS 设为包含目录的一部分
	find_path( opencc_INCLUDE_DIRS "opencc.h" PATHS "${opencc_SOURCE_DIR}/src" NO_DEFAULT_PATH REQUIRED )
	# message( STATUS "opencc_INCLUDE_DIRS = ${opencc_INCLUDE_DIRS}")

	# 调整三个辅助工具的保存目录, 都丢到 cbuild 某个不重要的地方 (若不设置则生成到根目录)
	set_target_properties(opencc opencc_dict opencc_phrase_extract
		PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY	"${opencc_BINARY_DIR}/build"
	)

	# 设置一个永远会被 BUILD 的自定义 TARGET 并让他依赖 Dictionaries 项目
	# 这里的 Dictionaries 项目是 opencc 用来生成字典 ocd2 文件的相关项目, 设为依赖可以
	# 确保 Dictionaries 永远在 DictionariesCopy 之前被构建
	add_custom_target(DictionariesCopy ALL DEPENDS Dictionaries)

	# 接下来设置一个针对 DictionariesCopy 完成编译后触发的自定义指令集合
	# 主要干的就是一些文件复制的工作
	add_custom_command(TARGET DictionariesCopy
		POST_BUILD
		COMMAND mkdir -p "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/db/dicts"
		COMMAND cp -f "${opencc_SOURCE_DIR}/data/config/s2t.json" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/db/dicts"
		COMMAND cp -f "${opencc_SOURCE_DIR}/data/config/t2s.json" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/db/dicts"
		COMMAND cp -f "${opencc_BINARY_DIR}/data/ST*.ocd2" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/db/dicts"
		COMMAND cp -f "${opencc_BINARY_DIR}/data/TS*.ocd2" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/db/dicts"
	)
endif()

#
# 使用 CPM 模块引入 uchardet 第三方库
#

CPMAddPackage(
	NAME uchardet
	OPTIONS		"BUILD_BINARY OFF"
	SOURCE_DIR	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/3rdparty/uchardet"
)

# 若 uchardet 引入成功, 则进行一系列的额外配置操作
if(uchardet_ADDED)
	# 确保 ${uchardet_SOURCE_DIR}/src 目录中存在 uchardet.h 文件, 
	# 将目录的路径保存到 uchardet_INCLUDE_DIRS 变量中 (声明为 REQUIRED 如果失败则终止)
	# 后面 common 等项目构建的时候需要将 uchardet_INCLUDE_DIRS 设为包含目录的一部分
	find_path( uchardet_INCLUDE_DIRS "uchardet.h" PATHS "${uchardet_SOURCE_DIR}/src" NO_DEFAULT_PATH REQUIRED )
	# message( STATUS "uchardet_INCLUDE_DIRS = ${uchardet_INCLUDE_DIRS}")
endif()

#
# 使用 CPM 模块引入 fmt 第三方库
#

CPMAddPackage(
	NAME fmt
	OPTIONS		"FMT_INSTALL OFF"
	SOURCE_DIR	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/3rdparty/fmt"
)

# 若 fmt 引入成功, 则进行一系列的额外配置操作
if(fmt_ADDED)
	# 确保 ${fmt_SOURCE_DIR}/include 目录中存在 fmt/core.h 文件, 
	# 将目录的路径保存到 fmt_INCLUDE_DIRS 变量中 (声明为 REQUIRED 如果失败则终止)
	# 后面 common 等项目构建的时候需要将 fmt_INCLUDE_DIRS 设为包含目录的一部分
	find_path( fmt_INCLUDE_DIRS "fmt/core.h" PATHS "${fmt_SOURCE_DIR}/include" NO_DEFAULT_PATH REQUIRED )
	# message( STATUS "fmt_INCLUDE_DIRS = ${fmt_INCLUDE_DIRS}")
	
	# 启用 FMT_HEADER_ONLY 宏定义
	set_property( CACHE GLOBAL_DEFINITIONS  PROPERTY VALUE "${GLOBAL_DEFINITIONS} -DFMT_HEADER_ONLY" )
endif()

#
# 使用 CPM 模块引入 utfcpp 第三方库
#

CPMAddPackage(
	NAME utfcpp
	SOURCE_DIR	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/3rdparty/utfcpp"
)

# 若 utfcpp 引入成功, 则进行一系列的额外配置操作
if(utfcpp_ADDED)
	# 确保 ${utfcpp_SOURCE_DIR}/source 目录中存在 utf8.h 文件, 
	# 将目录的路径保存到 utfcpp_INCLUDE_DIRS 变量中 (声明为 REQUIRED 如果失败则终止)
	# 后面 common 等项目构建的时候需要将 utfcpp_INCLUDE_DIRS 设为包含目录的一部分
	find_path( utfcpp_INCLUDE_DIRS "utf8.h" PATHS "${utfcpp_SOURCE_DIR}/source" NO_DEFAULT_PATH REQUIRED )
	# message( STATUS "utfcpp_INCLUDE_DIRS = ${utfcpp_INCLUDE_DIRS}")
endif()
