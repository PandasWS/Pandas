#
# setup
#

# 这里的 CMAKE_CURRENT_SOURCE_DIR 是系统变量, 表示当前 CMakeLists.txt 文件的绝对目录
# 下面将 CMAKE_CURRENT_SOURCE_DIR 的值赋予给 LOGIN_SOURCE_DIR, 使他们相等
set( LOGIN_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}  CACHE INTERNAL "" )

# 
# login server
#

# 如果已经定义了 BUILD_SERVERS 变量的话, 说明依赖条件已经具备
# 可以开始正式编译 login-server 的相关配置
# 此处 BUILD_SERVERS 变量是在 src/CMakeLists.txt 中定义的
if( BUILD_SERVERS )
	# 提示开始创建编译任务, 生成目标: login-server
	message( STATUS "Creating target login-server" )

	# 先找出 src/login 目录中的全部 hpp 文件, 保存到 LOGIN_HEADERS 变量
	file(GLOB LOGIN_HEADERS ${LOGIN_SOURCE_DIR}/*.hpp)

	# 再找出 src/login 目录中的全部 cpp 文件, 保存到 LOGIN_SOURCES 变量
	file(GLOB LOGIN_SOURCES ${LOGIN_SOURCE_DIR}/*.cpp)
	# message( STATUS "LOGIN_SOURCES="${LOGIN_SOURCES})

	# 设置依赖项目, 这里设置了依赖 common 这个项目
	set( DEPENDENCIES common )

	# 设置依赖的库文件
	# 包括 GLOBAL_LIBRARIES 以及依赖项目的库文件
	set( LIBRARIES ${GLOBAL_LIBRARIES} ${DEPENDENCIES} )
	# message( STATUS "GLOBAL_LIBRARIES="${GLOBAL_LIBRARIES})

	# 设置包含目录
	# 此处的 $(COMMON_BASE_INCLUDE_DIRS) 内容来自 src/common/CMakeLists.txt 中的定义
	# 此处的 ${RA_INCLUDE_DIRS} 内容来自 src/CMakeLists.txt 中的定义
	set( INCLUDE_DIRS ${GLOBAL_INCLUDE_DIRS} ${COMMON_BASE_INCLUDE_DIRS} ${RA_INCLUDE_DIRS} )
	# message( STATUS "GLOBAL_INCLUDE_DIRS="${GLOBAL_INCLUDE_DIRS})
	# message( STATUS "COMMON_BASE_INCLUDE_DIRS="${COMMON_BASE_INCLUDE_DIRS})

	# 设置宏定义
	# 此处的 $(COMMON_BASE_DEFINITIONS) 内容来自 src/common/CMakeLists.txt 中的定义
	set( DEFINITIONS "${GLOBAL_DEFINITIONS} ${COMMON_BASE_DEFINITIONS}" )
	# message( STATUS "GLOBAL_DEFINITIONS="${GLOBAL_DEFINITIONS})
	# message( STATUS "COMMON_BASE_DEFINITIONS="${COMMON_BASE_DEFINITIONS})

	# 设置用于编译的源码文件
	# 在 rAthena 原始的设置中, 还引入了 ${COMMON_BASE_HEADERS} ${COMMON_HEADERS} 这两个感觉用不上, 暂时移除也可以编译通过
	set( SOURCE_FILES ${LOGIN_HEADERS} ${LOGIN_SOURCES} )

	# 进行归类设置 - CMake 生成 Visual Studio 等可视化 IDE 的解决方案时使用
	# 将 COMMON_BASE_HEADERS 和 COMMON_HEADERS 中的文件, 统统归入名为 common 的文件组
	source_group( common FILES ${COMMON_BASE_HEADERS} ${COMMON_HEADERS} )

	# 进行归类设置 - CMake 生成 Visual Studio 等可视化 IDE 的解决方案时使用
	# 将 LOGIN_HEADERS 和 LOGIN_SOURCES 中的文件, 统统归入名为 login 的文件组
	source_group( login FILES ${LOGIN_HEADERS} ${LOGIN_SOURCES} )

	# =========================================================
	# BEGIN - 在 Pandas 中 Login-server 也需要 PCRE 库的支持
	# =========================================================
	# 如果有包含正则表达式库的话, 那么启用相关的宏定义、包含目录、库文件
	if( WITH_PCRE )
		message( STATUS "Enabled PCRE code" )
		set( LIBRARIES ${LIBRARIES} ${PCRE_LIBRARIES} )	# 库文件
		set( INCLUDE_DIRS ${INCLUDE_DIRS} ${PCRE_INCLUDE_DIRS} ) # 包含目录
		set( DEFINITIONS "${DEFINITIONS} -DPCRE_SUPPORT" ) # 宏定义
	else()
		message( STATUS "Disabled PCRE code" )
	endif()
	# =========================================================
	# END - 在 Pandas 中 Login-server 也需要 PCRE 库的支持
	# =========================================================

	# =========================================================
	# 以下进行真正的设置操作
	# =========================================================

	# 使用 SOURCE_FILES 的源代码文件, 生成一个名为 login-server 的 executable 目标
	add_executable( login-server ${SOURCE_FILES} )

	# 设置此项目依赖 DEPENDENCIES 变量中其他的顶级项目, 确保他们先被编译
	add_dependencies( login-server ${DEPENDENCIES} )
	
	# 将 INCLUDE_DIRS 变量中的内容作为我们的包含目录
	include_directories( ${INCLUDE_DIRS} )

	# 需要为 login-server 链接上 LIBRARIES 变量中的库文件, 其中包括我们依赖项目的编译产物
	target_link_libraries( login-server ${LIBRARIES} )

	# 设置编译时候的附加编译参数, 主要是一些宏定义
	# 备注: 在 DEFINITIONS 里面还有一个 -fno-strict-aliasing 参数,
	#      否则其实可以用其他的 CMake 指令来指定宏定义开关
	set_target_properties( login-server PROPERTIES COMPILE_FLAGS "${DEFINITIONS}" )
	# message( STATUS "DEFINITIONS="${DEFINITIONS})

	# 若启用了 INSTALL_COMPONENT_RUNTIME 的话, 那么执行一些和安装配置.
	# 以 INSTALL_COMPONENT_ 作为前缀的变量有两个, 一个是 RUNTIME 另一个是 DEVELOPMENT
	# 他们这两者的区别是: RUNTIME - 不包含源代码, 而 DEVELOPMENT - 包含源代码
	if( INSTALL_COMPONENT_RUNTIME )
		# 使用 cpack_add_component 指令, 定义一个名为 Runtime_loginserver 的策略:
		# 往一个名为 Runtime 的组中, 添加一个名为 login-server 描述也是 login-server 的组件
		cpack_add_component( Runtime_loginserver DESCRIPTION "login-server" DISPLAY_NAME "login-server" GROUP Runtime )
		
		# 将我们名为 login-server 的目标安装到 . 目录, 并与 Runtime_loginserver 策略关联
		install( TARGETS login-server DESTINATION "." COMPONENT Runtime_loginserver )
	endif( INSTALL_COMPONENT_RUNTIME )

	# 往名为 TARGET_LIST 的变量末尾追加本次的生成目标: login-server
	set( TARGET_LIST ${TARGET_LIST} login-server CACHE INTERNAL "" )

	# 提示整个 login-server 编译任务的创建过程到此结束
	message( STATUS "Creating target login-server - done" )
endif( BUILD_SERVERS )
