
#
# mapcache
#

# 如果已经定义了 WITH_ZLIB 变量的话, 那么说明 zlib 库存在
# 提示: 此变量应该是在 3rdparty/CMakeLists.txt 中定义的, 需要具体看看
if( WITH_ZLIB )
	# 那么新定义一个 BUILD_MAPCACHE 变量, 将其设为 ON (开启)
	option( BUILD_MAPCACHE "build mapcache executable" ON )
else()
	# 否则, 提示想要编译 mapcache 需要依赖 zlib 库
	message( STATUS "Disabled mapcache target (required ZLIB)" )
endif()

# 如果已经定义了 BUILD_MAPCACHE 变量的话, 说明依赖条件已经具备
# 可以开始正式编译 mapcache 的相关配置
# 
# 全部的项目中, 只有 tool 中的这些项目并不直接依赖 common
# 而是直接选择了 COMMON_MINI_HEADERS 定义的一些文件, 与自己所需要的一些文件混在一起现场编译
# 
# 所以此处比如设置 ${INCLUDE_DIRS} 的时候,
# ${COMMON_MINI_INCLUDE_DIRS} 变量里面的内容其实和 ${DEPENDENCIES} 是有重叠的,
# 因为在 ${COMMON_MINI_INCLUDE_DIRS} 中已经引用了 LIBCONFIG_INCLUDE_DIRS 和 BREAKPAD_INCLUDE_DIRS
if( BUILD_MAPCACHE )
	# 提示开始创建编译任务, 生成目标: mapcache
	message( STATUS "Creating target mapcache" )

	# 将此项目会用到的 common 项目的头文件清单存放到 COMMON_HEADERS 变量中
	# 此处的 $(COMMON_MINI_HEADERS) 内容来自 src/common/CMakeLists.txt 中的定义
	set( COMMON_HEADERS
		${COMMON_MINI_HEADERS}
		"${COMMON_SOURCE_DIR}/des.hpp"
		"${COMMON_SOURCE_DIR}/grfio.hpp"
		"${COMMON_SOURCE_DIR}/utils.hpp"
	)

	# 将此项目会用到的 common 项目的源码文件清单存放到 COMMON_SOURCES 变量中
	# 此处的 $(COMMON_MINI_SOURCES) 内容来自 src/common/CMakeLists.txt 中的定义
	set( COMMON_SOURCES
		${COMMON_MINI_SOURCES}
		"${COMMON_SOURCE_DIR}/des.cpp"
		"${COMMON_SOURCE_DIR}/grfio.cpp"
		"${COMMON_SOURCE_DIR}/utils.cpp"
	)

	# 将此项目相关的源代码文件, 存入 MAPCACHE_SOURCES 变量
	set( MAPCACHE_SOURCES
		"${CMAKE_CURRENT_SOURCE_DIR}/mapcache.cpp"
	)

	# 设置依赖项目, 这里设置了依赖 breakpad cryptopp-static yaml-cpp ryml libopencc libuchardet 这几个项目
	set( DEPENDENCIES breakpad cryptopp-static yaml-cpp ryml libopencc libuchardet )

	# 设置依赖的库文件
	# 包括 GLOBAL_LIBRARIES, ZLIB_LIBRARIES, 包括依赖项目的库文件
	set( LIBRARIES ${GLOBAL_LIBRARIES} ${ZLIB_LIBRARIES} ${DEPENDENCIES} )

	# 设置包含目录
	# 此处的 ${ZLIB_INCLUDE_DIRS} 是 zlib 依赖项目的包含目录
	# 此处的 ${COMMON_MINI_INCLUDE_DIRS} 是 common 依赖项目的包含目录
	# 此处的 ${RA_INCLUDE_DIRS} 内容来自 src/CMakeLists.txt 中的定义
	set( INCLUDE_DIRS ${GLOBAL_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS} ${COMMON_MINI_INCLUDE_DIRS} ${RA_INCLUDE_DIRS} )

	# 设置宏定义
	# 此处的 $(COMMON_MINI_DEFINITIONS) 内容来自 src/common/CMakeLists.txt 中的定义
	set( DEFINITIONS "${GLOBAL_DEFINITIONS} ${COMMON_MINI_DEFINITIONS}" )

	# 设置用于编译的源码文件
	set( SOURCE_FILES ${COMMON_HEADERS} ${COMMON_SOURCES} ${MAPCACHE_SOURCES} )
	
	# 进行归类设置 - CMake 生成 Visual Studio 等可视化 IDE 的解决方案时使用
	# 将 COMMON_HEADERS 和 COMMON_SOURCES 中的文件, 统统归入名为 common 的文件组
	source_group( common FILES ${COMMON_HEADERS} ${COMMON_SOURCES} )
	
	# 进行归类设置 - CMake 生成 Visual Studio 等可视化 IDE 的解决方案时使用
	# 将 MAPCACHE_SOURCES 中的文件, 统统归入名为 mapcache 的文件组
	# 提示: 此项目有仅一个 cpp 文件, 没有 hpp 头文件, 所以并没有类似 MAPCACHE_HEADERS 的存在
	source_group( mapcache FILES ${MAPCACHE_SOURCES} )

	# =========================================================
	# 以下进行真正的设置操作
	# =========================================================
	# 使用 SOURCE_FILES 的源代码文件, 生成一个名为 mapcache 的 executable 目标
	add_executable( mapcache ${SOURCE_FILES} )

	# 设置此项目依赖 DEPENDENCIES 变量中其他的顶级项目, 确保他们先被编译
	add_dependencies( mapcache ${DEPENDENCIES} )

	# 将 INCLUDE_DIRS 变量中的内容作为我们的包含目录
	include_directories( ${INCLUDE_DIRS} )

	# 需要为 mapcache 链接上 LIBRARIES 变量中的库文件, 其中包括我们依赖项目的编译产物
	target_link_libraries( mapcache ${LIBRARIES} )

	# 设置编译时候的附加编译参数, 主要是一些宏定义
	# 备注: 在 DEFINITIONS 里面还有一个 -fno-strict-aliasing 参数,
	#      否则其实可以用其他的 CMake 指令来指定宏定义开关
	set_target_properties( mapcache PROPERTIES COMPILE_FLAGS "${DEFINITIONS}" )

	# 若启用了 INSTALL_COMPONENT_RUNTIME 的话, 那么执行一些和安装配置.
	# 以 INSTALL_COMPONENT_ 作为前缀的变量有两个, 一个是 RUNTIME 另一个是 DEVELOPMENT
	# 他们这两者的区别是: RUNTIME - 不包含源代码, 而 DEVELOPMENT - 包含源代码
	if( INSTALL_COMPONENT_RUNTIME )
		# 使用 cpack_add_component 指令, 定义一个名为 Runtime_mapcache 的策略:
		# 往一个名为 Runtime 的组中, 添加一个名为 mapcache 描述为 mapcache generator 的组件
		cpack_add_component( Runtime_mapcache DESCRIPTION "mapcache generator" DISPLAY_NAME "mapcache" GROUP Runtime )
		
		# 将我们名为 mapcache 的目标安装到 . 目录, 并与 Runtime_mapcache 策略关联
		install( TARGETS mapcache DESTINATION "." COMPONENT Runtime_mapcache )
	endif( INSTALL_COMPONENT_RUNTIME )

	# 往名为 TARGET_LIST 的变量末尾追加本次的生成目标: mapcache
	set( TARGET_LIST ${TARGET_LIST} mapcache  CACHE INTERNAL "" )

	# 提示整个 mapcache 编译任务的创建过程到此结束
	message( STATUS "Creating target mapcache - done" )
endif( BUILD_MAPCACHE )
