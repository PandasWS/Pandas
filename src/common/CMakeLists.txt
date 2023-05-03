
#
# Create version.hpp
#

# 提示开始创建 version.hpp 文件
message( STATUS "Creating version.hpp" )

# 如果已经定义了 GIT_VERSION 变量
# 那么说明在 {project_root}/CMakeLists.txt 中, 通过 get_git_version 方法
# 判断得出当前代码仓库是使用 git 进行版本控制的
if(GIT_VERSION)
	# 这里的 ${GIT_VERSION} 是 get_git_version 函数填充的<master>分支最新一个版本的 hash 版本号
	# 是一个完整的 32 位 hash 值. 这里我们截断一下, 只去它前 10 位放到 SHORT_GIT_VERSION 变量中去
	string(SUBSTRING ${GIT_VERSION} 0 10 SHORT_GIT_VERSION)

	# 这里的 ${GIT_HEAD_VERSION} 是 get_git_version 函数填充的<当前>分支最新一个版本的 hash 版本号
	# 是一个完整的 32 位 hash 值. 这里我们截断一下, 只去它前 10 位放到 SHORT_GIT_HEAD_VERSION 变量中去
	string(SUBSTRING ${GIT_HEAD_VERSION} 0 10 SHORT_GIT_HEAD_VERSION)

	# 将上面获得的两个缩短版本的 hash 版本号, 通过 _ 拼接在一起, 保存到 GIT_STR_VERSIONS 变量中去
	string(CONCAT GIT_STR_VERSIONS ${SHORT_GIT_VERSION} "_" ${SHORT_GIT_HEAD_VERSION})
	#message( STATUS "git version=${GIT_STR_VERSIONS}" )

	# 往 ${CMAKE_CURRENT_BINARY_DIR}/version.hpp 文件中写入相关代码
	# 如果未定义 SVNVERSION 那么则将 SVNVERSION 定义成 ${GIT_STR_VERSIONS} 变量的值
	# 
	# 这里的 CMAKE_CURRENT_BINARY_DIR 变量是 CMake 内部变量,
	# 表示 build 目录树中, 与当前 CMakeLists.txt 所在目录对应的路径...
	# 
	# 例如:
	# 假设我们在 {project_root} 创建了 cbuild 目录, 然后在里面进行了 cmake -G "Unix Makefiles" .. 指令
	# 且当前我们 CMakeLists.txt 相对于 {project_root} 是在 src/common 目录下
	# 那么 ${CMAKE_CURRENT_BINARY_DIR} 在这里的值就是: {project_root}/cbuild/src/common
	file( WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.hpp
		"#ifndef SVNVERSION\n#define SVNVERSION ${GIT_STR_VERSIONS}\n#endif\n" )
elseif( SVNVERSION )
	# 否则如果没有定义 GIT_VERSION 但是定义了 SVNVERSION 的话
	# 那么直接往 ${CMAKE_CURRENT_BINARY_DIR}/version.hpp 写入相关代码
	# 将 SVNVERSION 定义成此处读取出来的值 ${SVNVERSION} 版本号
	#
	# 备注:
	# 在当前的版本中 {project_root}/CMakeLists.txt 已经不会再调用 get_svn_version 去填充该变量了
	# 所以这个分支其实目前永远不会被执行.
	file( WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.hpp
		"#ifndef SVNVERSION\n#define SVNVERSION ${SVNVERSION}\n#endif\n" )
else()
	# 如果 GIT_VERSION 和 SVNVERSION 都没有被定义, 那么 version.hpp 文件内容直接为空
	file( WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.hpp "" )
endif()

# 后续进行编译时, 将 ${CMAKE_CURRENT_BINARY_DIR} 也追加到
# ${GLOBAL_INCLUDE_DIRS} 的末尾, 作为公用包含目录的一部分
set( GLOBAL_INCLUDE_DIRS ${GLOBAL_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR} CACHE INTERNAL "" )

# 将 SVNVERSION 的值设置为自己, 这一行看起来好像没有任何有价值的意义.
# TODO: 后面可以研究一下把它删掉会发生什么事情?
set( SVNVERSION ${SVNVERSION} CACHE STRING "SVN version of the source code" )

# 若启用了 INSTALL_COMPONENT_DEVELOPMENT 的话, 那么执行一些和安装配置.
# 以 INSTALL_COMPONENT_ 作为前缀的变量有两个, 一个是 RUNTIME 另一个是 DEVELOPMENT
# 他们这两者的区别是: RUNTIME - 不包含源代码, 而 DEVELOPMENT - 包含源代码
if( INSTALL_COMPONENT_DEVELOPMENT )
	# 将 version.hpp 安装到 src/common 目录, 并与 Development_base 策略关联
	install(
		FILES ${CMAKE_CURRENT_BINARY_DIR}/version.hpp
		DESTINATION "src/common"
		COMPONENT Development_base
	)
endif( INSTALL_COMPONENT_DEVELOPMENT )

# 提示整个创建 version.hpp 的过程到此结束
message( STATUS "Creating version.hpp - done" )

#
# setup
#

# 如果定义了 WIN32 的话, 说明代码希望编译在 Windows 环境
if( WIN32 )
	# 那么将 PROJECT_LIBS 设置为 Ws2_32.lib
	# 但是目前发现整个项目中都没有使用到 PROJECT_LIBS 这个变量,
	# 且这个 PROJECT_LIBS 应该也不是 CMake 的系统变量, 感觉这块的分支应该是没测试过的
	set( PROJECT_LIBS Ws2_32.lib )

	# 如果使用的是 MSCV 编译器, 那么追加 _WINSOCK_DEPRECATED_NO_WARNINGS 宏定义开关
	if( MSVC )
		add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
	endif()

	# 如果是在 Windows 环境的话, 设置 COMMON_ADDITIONALL_CPP 的内容为 winapi.cpp 文件
	set( COMMON_ADDITIONALL_CPP	"${COMMON_SOURCE_DIR}/winapi.cpp" )

	# 如果是在 Windows 环境的话, 设置 COMMON_ADDITIONALL_HPP 的内容为 winapi.hpp 文件
	set( COMMON_ADDITIONALL_HPP	"${COMMON_SOURCE_DIR}/winapi.hpp" )
endif()

# 这里的 CMAKE_CURRENT_SOURCE_DIR 是系统变量, 表示当前 CMakeLists.txt 文件的绝对目录
# 下面将 CMAKE_CURRENT_SOURCE_DIR 的值赋予给 COMMON_SOURCE_DIR, 使他们相等
set( COMMON_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE PATH "common source directory" )
#message( STATUS "DEBUG COMMON_SOURCE_DIR=${COMMON_SOURCE_DIR}" )

# 将 COMMON_SOURCE_DIR 提升为高级变量
# 高级变量指的是那些在 CMake GUI 中，只有当“显示高级选项”被打开时才会被显示的变量
mark_as_advanced( COMMON_SOURCE_DIR )

# 定义一些无论是 common 还是 common_base 都会用到的头文件
# 将他们存放到一个名为 COMMON_ALL_HEADERS 的变量中
set( COMMON_ALL_HEADERS
	"${CMAKE_CURRENT_BINARY_DIR}/version.hpp"
	"${COMMON_SOURCE_DIR}/cbasetypes.hpp"
	"${COMMON_SOURCE_DIR}/mmo.hpp"
)

#
# common mini defines 
#

# 以下的这四个变量, 仅在此进行赋值, 并不进行任何使用
# COMMON_MINI_HEADERS, COMMON_MINI_SOURCES, COMMON_MINI_INCLUDE_DIRS, COMMON_MINI_DEFINITIONS
# 它们目前只有一个使用者, 即: src/tools 项目的 CMakeLists.txt 中才引用到
# 
# TODO: tools 中的 mapcache 项目其实也依赖 zlib,
# 是否在那里可以直接用 COMMON_BASE_ 开头的一系列变量, 而不是 COMMON_MINI_ 开头的变量
# 可能关键看一下 MINICORE 宏定义的覆盖范围

set( COMMON_MINI_HEADERS
	${COMMON_ALL_HEADERS}
	"${COMMON_SOURCE_DIR}/core.hpp"
	"${COMMON_SOURCE_DIR}/malloc.hpp"
	"${COMMON_SOURCE_DIR}/showmsg.hpp"
	"${COMMON_SOURCE_DIR}/strlib.hpp"
	"${COMMON_SOURCE_DIR}/assistant.hpp"
	"${COMMON_SOURCE_DIR}/utf8.hpp"
	"${COMMON_SOURCE_DIR}/crashdump.hpp"
	"${COMMON_SOURCE_DIR}/cryptopp.hpp"
	"${COMMON_SOURCE_DIR}/translate.hpp"
	"${COMMON_SOURCE_DIR}/database.hpp"
	"${COMMON_SOURCE_DIR}/performance.hpp"
	"${COMMON_SOURCE_DIR}/processmutex.hpp"
	"${COMMON_SOURCE_DIR}/configparser.hpp"
	${LIBCONFIG_HEADERS} # needed by showmsg.hpp
	${BREAKPAD_HEADERS}
	${COMMON_ADDITIONALL_HPP} # needed by Windows
	CACHE INTERNAL ""
)

set( COMMON_MINI_SOURCES
	"${COMMON_SOURCE_DIR}/core.cpp"
	"${COMMON_SOURCE_DIR}/malloc.cpp"
	"${COMMON_SOURCE_DIR}/showmsg.cpp"
	"${COMMON_SOURCE_DIR}/strlib.cpp"
	"${COMMON_SOURCE_DIR}/assistant.cpp"
	"${COMMON_SOURCE_DIR}/utf8.cpp"
	"${COMMON_SOURCE_DIR}/crashdump.cpp"
	"${COMMON_SOURCE_DIR}/cryptopp.cpp"
	"${COMMON_SOURCE_DIR}/translate.cpp"
	"${COMMON_SOURCE_DIR}/database.cpp"
	"${COMMON_SOURCE_DIR}/performance.cpp"
	"${COMMON_SOURCE_DIR}/processmutex.cpp"
	${LIBCONFIG_SOURCES} # needed by showmsg.cpp
	${BREAKPAD_SOURCES}
	${COMMON_ADDITIONALL_CPP} # needed by Windows
	CACHE INTERNAL ""
)

set( COMMON_MINI_INCLUDE_DIRS
	${LIBCONFIG_INCLUDE_DIRS}
	${BREAKPAD_INCLUDE_DIRS}
	${YAML_INCLUDE_DIRS}
	${opencc_INCLUDE_DIRS}
	${uchardet_INCLUDE_DIRS}
	CACHE INTERNAL ""
)

set( COMMON_MINI_DEFINITIONS
	"-DMINICORE ${LIBCONFIG_DEFINITIONS}"
	CACHE INTERNAL ""
)

#
# common_base
#

# 如果已经定义了 WITH_ZLIB 变量的话, 那么说明 zlib 库存在
# 提示: 此变量应该是在 3rdparty/CMakeLists.txt 中定义的 CONFIGURE_WITH_LOCAL_OR_SYSTEM 函数
# 并在 3rdparty/zlib/CMakeLists.txt 中进行具体的 CONFIGURE_WITH_LOCAL_OR_SYSTEM 调用
if( WITH_ZLIB )
	# 提示开始创建编译任务, 生成目标: common_base
	message( STATUS "Creating target common_base" )

	set( COMMON_BASE_HEADERS
		${COMMON_ALL_HEADERS}
		"${COMMON_SOURCE_DIR}/conf.hpp"
		"${COMMON_SOURCE_DIR}/core.hpp"
		"${COMMON_SOURCE_DIR}/database.hpp"
		"${COMMON_SOURCE_DIR}/db.hpp"
		"${COMMON_SOURCE_DIR}/des.hpp"
		"${COMMON_SOURCE_DIR}/ers.hpp"
		"${COMMON_SOURCE_DIR}/grfio.hpp"
		"${COMMON_SOURCE_DIR}/malloc.hpp"
		"${COMMON_SOURCE_DIR}/mapindex.hpp"
		"${COMMON_SOURCE_DIR}/md5calc.hpp"
		"${COMMON_SOURCE_DIR}/nullpo.hpp"
		"${COMMON_SOURCE_DIR}/random.hpp"
		"${COMMON_SOURCE_DIR}/showmsg.hpp"
		"${COMMON_SOURCE_DIR}/socket.hpp"
		"${COMMON_SOURCE_DIR}/strlib.hpp"
		"${COMMON_SOURCE_DIR}/timer.hpp"
		"${COMMON_SOURCE_DIR}/utils.hpp"
		"${COMMON_SOURCE_DIR}/msg_conf.hpp"
		"${COMMON_SOURCE_DIR}/cli.hpp"
		"${COMMON_SOURCE_DIR}/utilities.hpp"
		"${COMMON_SOURCE_DIR}/future.hpp"
		"${COMMON_SOURCE_DIR}/jobqueue.hpp"
		"${COMMON_SOURCE_DIR}/assistant.hpp"
		"${COMMON_SOURCE_DIR}/utf8.hpp"
		"${COMMON_SOURCE_DIR}/crashdump.hpp"
		"${COMMON_SOURCE_DIR}/cryptopp.hpp"
		"${COMMON_SOURCE_DIR}/translate.hpp"
		"${COMMON_SOURCE_DIR}/performance.hpp"
		"${COMMON_SOURCE_DIR}/processmutex.hpp"
		"${COMMON_SOURCE_DIR}/configparser.hpp"
		${LIBCONFIG_HEADERS} # needed by conf.hpp/showmsg.hpp
		${BREAKPAD_HEADERS}
		${COMMON_ADDITIONALL_HPP} # needed by Windows
		CACHE INTERNAL "common_base headers"
	)

	set( COMMON_BASE_SOURCES
		"${COMMON_SOURCE_DIR}/conf.cpp"
		"${COMMON_SOURCE_DIR}/core.cpp"
		"${COMMON_SOURCE_DIR}/database.cpp"
		"${COMMON_SOURCE_DIR}/db.cpp"
		"${COMMON_SOURCE_DIR}/des.cpp"
		"${COMMON_SOURCE_DIR}/ers.cpp"
		"${COMMON_SOURCE_DIR}/grfio.cpp"
		"${COMMON_SOURCE_DIR}/malloc.cpp"
		"${COMMON_SOURCE_DIR}/mapindex.cpp"
		"${COMMON_SOURCE_DIR}/md5calc.cpp"
		"${COMMON_SOURCE_DIR}/nullpo.cpp"
		"${COMMON_SOURCE_DIR}/random.cpp"
		"${COMMON_SOURCE_DIR}/showmsg.cpp"
		"${COMMON_SOURCE_DIR}/socket.cpp"
		"${COMMON_SOURCE_DIR}/strlib.cpp"
		"${COMMON_SOURCE_DIR}/timer.cpp"
		"${COMMON_SOURCE_DIR}/utils.cpp"
		"${COMMON_SOURCE_DIR}/msg_conf.cpp"
		"${COMMON_SOURCE_DIR}/cli.cpp"
		"${COMMON_SOURCE_DIR}/utilities.cpp"
		"${COMMON_SOURCE_DIR}/future.cpp"
		"${COMMON_SOURCE_DIR}/assistant.cpp"
		"${COMMON_SOURCE_DIR}/utf8.cpp"
		"${COMMON_SOURCE_DIR}/crashdump.cpp"
		"${COMMON_SOURCE_DIR}/cryptopp.cpp"
		"${COMMON_SOURCE_DIR}/translate.cpp"
		"${COMMON_SOURCE_DIR}/performance.cpp"
		"${COMMON_SOURCE_DIR}/processmutex.cpp"
		${LIBCONFIG_SOURCES} # needed by conf.cpp/showmsg.cpp
		${BREAKPAD_SOURCES}
		${COMMON_ADDITIONALL_CPP} # needed by Windows
		CACHE INTERNAL "common_base sources"
	)

	set( COMMON_BASE_INCLUDE_DIRS
		${LIBCONFIG_INCLUDE_DIRS}
		${YAML_INCLUDE_DIRS}
		${BREAKPAD_INCLUDE_DIRS}
		${opencc_INCLUDE_DIRS}
		${uchardet_INCLUDE_DIRS}
		CACHE INTERNAL "common_base include dirs"
	)

	set( COMMON_BASE_DEFINITIONS
		${LIBCONFIG_DEFINITIONS}
		CACHE INTERNAL "common_base definitions"
	)

	# 设置依赖项目, 这里设置了依赖 yaml-cpp ryml breakpad cryptopp-static libopencc libuchardet 这几个项目
	set( DEPENDENCIES breakpad yaml-cpp ryml cryptopp-static libopencc libuchardet )

	# 设置依赖的库文件
	# 包括 GLOBAL_LIBRARIES, ZLIB_LIBRARIES, 包括依赖项目的库文件
	set( LIBRARIES ${GLOBAL_LIBRARIES} ${ZLIB_LIBRARIES} ${DEPENDENCIES} )

	# 设置包含目录
	# 此处的 ${ZLIB_INCLUDE_DIRS} 是 zlib 依赖项目的包含目录
	# 此处的 ${RA_INCLUDE_DIRS} 内容来自 src/CMakeLists.txt 中的定义
	set( INCLUDE_DIRS ${GLOBAL_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS} ${COMMON_BASE_INCLUDE_DIRS} ${RA_INCLUDE_DIRS} )

	# 设置宏定义
	set( DEFINITIONS "${GLOBAL_DEFINITIONS} ${COMMON_BASE_DEFINITIONS}" )

	# 设置用于编译的源码文件
	set( SOURCE_FILES ${COMMON_BASE_HEADERS} ${COMMON_BASE_SOURCES} )

	# 进行归类设置 - CMake 生成 Visual Studio 等可视化 IDE 的解决方案时使用
	# 将 COMMON_BASE_HEADERS 和 COMMON_BASE_SOURCES 中的文件, 统统归入名为 common 的文件组
	source_group( common FILES ${COMMON_BASE_HEADERS} ${COMMON_BASE_SOURCES} )

	# =========================================================
	# 以下进行真正的设置操作
	# =========================================================

	# 使用 SOURCE_FILES 的源代码文件, 生成一个名为 common_base 的静态库目标
	add_library( common_base STATIC ${SOURCE_FILES} )
	#message( STATUS "common_base LIBRARIES=${LIBRARIES}, DEFINITIONS=${DEFINITIONS}")

	# 设置此项目依赖 DEPENDENCIES 变量中其他的顶级项目, 确保他们先被编译
	add_dependencies( common_base ${DEPENDENCIES} )

	# 将 INCLUDE_DIRS 变量中的内容作为我们的包含目录
	include_directories( ${INCLUDE_DIRS} )

	# 需要为 common_base 链接上 LIBRARIES 变量中的库文件, 其中包括我们依赖项目的编译产物
	target_link_libraries( common_base ${LIBRARIES} )

	# 设置编译时候的附加编译参数, 主要是一些宏定义
	# 备注: 在 DEFINITIONS 里面还有一个 -fno-strict-aliasing 参数,
	#      否则其实可以用其他的 CMake 指令来指定宏定义开关
	set_target_properties( common_base PROPERTIES COMPILE_FLAGS "${DEFINITIONS}" )

	# 将 HAVE_common_base 定义为开启, 以便后面构建 common 项目时候做判断
	set( HAVE_common_base ON  CACHE INTERNAL "" )

	# 往名为 TARGET_LIST 的变量末尾追加本次的生成目标: common_base
	set( TARGET_LIST ${TARGET_LIST} common_base  CACHE INTERNAL "" )

	# 提示整个 common_base 编译任务的创建过程到此结束
	message( STATUS "Creating target common_base - done" )
else()
	# 否则, 提示想要编译 common_base 需要依赖 zlib 库
	message( STATUS "Skipping target common_base (requires ZLIB)" )

	# 同时取消掉对 HAVE_common_base 的定义
	unset( HAVE_common_base CACHE )
endif()

#
# common
#

# 必须已经定义了 HAVE_common_base 和 WITH_MYSQL 才能编译 common 项目
# 提示: WITH_MYSQL 变量应该是在 3rdparty/CMakeLists.txt 中定义的 CONFIGURE_WITH_LOCAL_OR_SYSTEM 函数
# 并在 3rdparty/mysql/CMakeLists.txt 中进行具体的 CONFIGURE_WITH_LOCAL_OR_SYSTEM 调用
#
# common 和 common_base 的区别在于一个依赖 mysql, 而另外一个不依赖
# 不过其实整个工程中目前依赖 common_base 的只有 common 自己, 其他的服务端项目都仅依赖 common
# 猜测: 这 common 和 common_base 两者的区分, 可能是个历史遗留产物 (为了兼容以前 txt 作为数据库的老版本?)
if( HAVE_common_base AND WITH_MYSQL )
	# 提示开始创建编译任务, 生成目标: common
	message( STATUS "Creating target common" )

	set( COMMON_HEADERS
		${COMMON_ALL_HEADERS}
		"${CMAKE_CURRENT_SOURCE_DIR}/sql.hpp"
		CACHE INTERNAL "common headers"
	)

	set( COMMON_SOURCES
		"${CMAKE_CURRENT_SOURCE_DIR}/sql.cpp"
		CACHE INTERNAL "common sources"
	)

	# 设置依赖项目
	# 这里设置了依赖 common_base 这个项目
	set( DEPENDENCIES common_base )

	# 设置依赖的库文件
	# 包括 GLOBAL_LIBRARIES, MYSQL_LIBRARIES, 包括依赖项目的库文件
	set( LIBRARIES ${GLOBAL_LIBRARIES} ${MYSQL_LIBRARIES} ${DEPENDENCIES} )

	# 设置包含目录
	# 此处的 ${MYSQL_INCLUDE_DIRS} 是 mysql 依赖项目的包含目录
	set( INCLUDE_DIRS ${GLOBAL_INCLUDE_DIRS} ${MYSQL_INCLUDE_DIRS} )

	# 设置宏定义
	set( DEFINITIONS "${GLOBAL_DEFINITIONS}" )

	# 设置用于编译的源码文件
	set( SOURCE_FILES ${COMMON_HEADERS} ${COMMON_SOURCES} )

	# 进行归类设置 - CMake 生成 Visual Studio 等可视化 IDE 的解决方案时使用
	# 将 COMMON_HEADERS 和 COMMON_SOURCES 中的文件, 统统归入名为 common 的文件组
	source_group( common FILES ${COMMON_HEADERS} ${COMMON_SOURCES} )

	# =========================================================
	# 以下进行真正的设置操作
	# =========================================================

	# 使用 SOURCE_FILES 的源代码文件, 生成一个名为 common 的静态库目标
	add_library( common STATIC ${SOURCE_FILES} )
	#message( STATUS "common LIBRARIES=${LIBRARIES}, DEPENDENCIES=${DEPENDENCIES} DEFINITIONS=${DEFINITIONS}")

	# 设置此项目依赖 DEPENDENCIES 变量中其他的顶级项目, 确保他们先被编译
	add_dependencies( common ${DEPENDENCIES} )

	# 将 INCLUDE_DIRS 变量中的内容作为我们的包含目录
	include_directories( ${INCLUDE_DIRS} )

	# 需要为 common 链接上 LIBRARIES 变量中的库文件, 其中包括我们依赖项目的编译产物
	target_link_libraries( common ${LIBRARIES} )

	# 设置编译时候的附加编译参数, 主要是一些宏定义
	# 备注: 在 DEFINITIONS 里面还有一个 -fno-strict-aliasing 参数,
	#      否则其实可以用其他的 CMake 指令来指定宏定义开关
	set_target_properties( common PROPERTIES COMPILE_FLAGS "${DEFINITIONS}" )

	# 将 HAVE_common 定义为开启
	set( HAVE_common ON  CACHE INTERNAL "" )

	# 往名为 TARGET_LIST 的变量末尾追加本次的生成目标: common
	set( TARGET_LIST ${TARGET_LIST} common  CACHE INTERNAL "" )

	# 提示整个 common 编译任务的创建过程到此结束
	message( STATUS "Creating target common - done" )
else()
	# 否则, 提示想要编译 common 需要依赖 common_base 和 mysql 库
	message( FATAL_ERROR "Stopping target common (requires common_base and MYSQL)" )
	# 由于上面使用了 FATAL_ERROR 所以其实后面也没必要再 unset 掉 HAVE_common 了
endif()
