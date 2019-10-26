# - Check which library is needed to link a C function
# find_function_library( <function> <variable> [<library> ...] )
#
# Check which library provides the <function>.
# Sets <variable> to 0 if found in the global libraries.
# Sets <variable> to the library path if found in the provided libraries.
# Raises a FATAL_ERROR if not found.
#
# The following variables may be set before calling this macro to
# modify the way the check is run:
#
#  CMAKE_REQUIRED_FLAGS = string of compile command line flags
#  CMAKE_REQUIRED_DEFINITIONS = list of macros to define (-DFOO=bar)
#  CMAKE_REQUIRED_INCLUDES = list of include directories
#  CMAKE_REQUIRED_LIBRARIES = list of libraries to link

# 引入 CMake 的 CheckFunctionExists 模块, 里面提供了 CHECK_FUNCTION_EXISTS 函数
include( CheckFunctionExists )

# 定义一个名为 find_function_library 的函数
# 使用方法: find_function_library( <function> <variable> [<library> ...] )
macro( find_function_library FUNC VAR )

	# 开始循环查找所有可变的 library 参数, 逐个放入 LIB 变量
	foreach( LIB IN ITEMS ${ARGN} )

		# 显示提示: 开始在 ${LIB} 中搜索 ${FUNC} 变量
		message( STATUS "Looking for ${FUNC} in ${LIB} library" )

		# 寻找 ${LIB} 的路径并保存到 ${LIB}_LIBRARY 变量中去
		find_library( ${LIB}_LIBRARY ${LIB} )

		# 根据 find_library 的文档, 如果找不到则变量将会返回 ${LIB}_LIBRARY-NOTFOUND
		# 如果无法定位到所需要的库, 那么直接报错然后退出 
		if ( ${${LIB}_LIBRARY} MATCHES "${LIB}_LIBRARY-NOTFOUND")
			message( FATAL_ERROR "Library ${LIB} not found" )
		endif()

		# 将 ${LIB}_LIBRARY 提升为高级变量
		# 高级变量指的是那些在 CMake GUI 中，只有当“显示高级选项”被打开时才会被显示的变量
		mark_as_advanced( ${LIB}_LIBRARY )

		# 如果 ${LIB}_LIBRARY 变量已经被设置了内容
		if( ${LIB}_LIBRARY )

			# 清空 ${VAR} 变量的内容 (这里的 ${VAR} 就是 find_function_library 的函数返回值)
			unset( ${VAR} CACHE )

			# 将 ${LIB}_LIBRARY 的值替换到 CMAKE_REQUIRED_LIBRARIES 变量中
			set( CMAKE_REQUIRED_LIBRARIES ${${LIB}_LIBRARY} )

			# 从 CMAKE_REQUIRED_LIBRARIES 指定的库中确认给定的 ${FUNC} 是否存在
			# 如果存在则设置 ${VAR} 的值为 1, 否则不设置 ${VAR} 变量
			CHECK_FUNCTION_EXISTS( ${FUNC} ${VAR} )
			
			# 将 CMAKE_REQUIRED_LIBRARIES 变量的内容设置为空
			set( CMAKE_REQUIRED_LIBRARIES )

			# 如果 ${VAR} 的内容 (在此处即: CHECK_FUNCTION_EXISTS 的返回值已经有被定义)
			if( ${VAR} )
				
				# 那么提示已经在位于 ${LIB}_LIBRARY 路径的 ${LIB} 库中找到 ${FUNC} 函数
				message( STATUS "Found ${FUNC} in ${LIB}: ${${LIB}_LIBRARY}" )

				# 将 ${LIB}_LIBRARY 的值存入到 ${VAR} 变量
				# 在此 ${VAR} 预期作为 find_function_library 的函数返回值
				set( ${VAR} ${${LIB}_LIBRARY} CACHE INTERNAL "Found ${FUNC} in ${LIB}" )

				# 已经找到了这个 ${FUNC} 关联的 library 路径后, 就没必要继续搜索下一个库了
				break()

			endif( ${VAR} )
			
		endif( ${LIB}_LIBRARY )

	endforeach()

	# 如果用来作为函数返回值的 ${VAR} 并没有被定义
	if( NOT ${VAR} )

		# 那么提示我们无法找到 ${FUNC} 对应的 library 路径, 并终止退出
		message( FATAL_ERROR "Function ${FUNC} not found" )
		
	endif()

endmacro( find_function_library )
