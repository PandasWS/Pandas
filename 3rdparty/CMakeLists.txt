
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


add_subdirectory( libconfig )
add_subdirectory( mysql )
add_subdirectory( pcre )
add_subdirectory( zlib )
add_subdirectory( yaml-cpp )
add_subdirectory( breakpad )
add_subdirectory( cryptopp )
