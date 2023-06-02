#
# setup and static libraries
#

set(RA_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH
	"rathena include directory"
)
mark_as_advanced( RA_INCLUDE_DIRS )

# 添加外部目录 common
# 里面最终会生成两个项目, 一个不依赖 MYSQL 的 common_base
# 以及另外一个依赖 MYSQL 的 common
add_subdirectory( common )

# 如果依赖 MYSQL 的 common 被成功编译 (因此定义了 HAVE_common 变量)
if( HAVE_common )
	# 那么立刻声明 BUILD_SERVERS 变量, 准许后续三个服务端主程序生成
	option( BUILD_SERVERS "build server executables" ON )
else()
	# 否则那么提示本次编译将禁用三个服务端主程序的生成
	message( STATUS "Disabled server targets (requires common)" )
endif()

#
# targets
#

add_subdirectory( login )
add_subdirectory( char )
add_subdirectory( map )
add_subdirectory( web )
add_subdirectory( tool )
