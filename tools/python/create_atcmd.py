'''
//===== Pandas Python Script =================================
//= 管理员指令添加助手
//===== By: ================================================== 
//= Sola丶小克
//===== Current Version: ===================================== 
//= 1.0
//===== Description: ========================================= 
//= 此脚本用于快速建立一个新的管理员指令 (At Command)
//===== Additional Comments: ================================= 
//= 1.0 首个版本. [Sola丶小克]
//============================================================

// PYHELP - ATCMD - INSERT POINT - <Section 1 / 11>
pandas.hpp @ 宏定义

// PYHELP - ATCMD - INSERT POINT - <Section 2>
atcommand.cpp @ ACMD_FUNC 指令实际代码

// PYHELP - ATCMD - INSERT POINT - <Section 3>
atcommand.cpp @ ACMD_DEF 指令导出
'''

# -*- coding: utf-8 -*-

import environment
environment.initialize()

import os
from enum import IntEnum

from libs import Common, Injecter, Inputer, Message

# 工程文件的主目录相对此脚本文件的位置
project_slndir = '../../'
slndir_path = os.path.abspath(project_slndir)

class InjectPoint(IntEnum):
    PANDAS_SWITCH_DEFINE = 1
    ATCMD_FUNC = 2
    ATCMD_DEF = 3

def insert_atcmd(inject, options):
    define = options['define']
    funcname = options['funcname']
    cmdname = options['cmdname']
    
    # pandas.hpp @ 宏定义
    inject.insert(InjectPoint.PANDAS_SWITCH_DEFINE, [
        '',
        '\t// 是否启用 %s 管理员指令 [维护者昵称]' % cmdname,
        '\t// TODO: 请在此填写此管理员指令的说明',
        '\t#define %s' % define
    ])
    
    # atcommand.cpp @ ACMD_FUNC 指令实际代码
    inject.insert(InjectPoint.ATCMD_FUNC, [
        '#ifdef %s' % define,
        '/* ===========================================================',
        ' * 指令: %s' % cmdname,
        ' * 描述: 请在此补充该脚本指令的说明',
        ' * 用法: @%s' % cmdname,
        ' * 作者: 维护者昵称',
        ' * -----------------------------------------------------------*/',
        'ACMD_FUNC(%s) {' % funcname,
        '\t// TODO: 请在此填写管理员指令的实现代码',
        '\treturn 0;',
        '}',
        '#endif // %s' % define,
        ''
    ])
    
    # atcommand.cpp @ ACMD_DEF 指令导出
    
    if funcname == cmdname:
        defcontent = '\t\tACMD_DEF(%s),\t\t\t// 在此写上管理员指令说明 [维护者昵称]' % (funcname)
    else:
        defcontent = '\t\tACMD_DEF2(%s,"%s"),\t\t\t// 在此写上管理员指令说明 [维护者昵称]' % (cmdname, funcname)
    
    inject.insert(InjectPoint.ATCMD_DEF, [
        '#ifdef %s' % define,
        defcontent,
        '#endif // %s' % define
    ])

def guide(inject):

    define = Inputer().requireText({
        'tips' : '请输入该管理员指令的宏定义开关名称 (Pandas_AtCommand_的末尾部分)',
        'prefix' : 'Pandas_AtCommand_'
    })
    
    # --------

    funcname = Inputer().requireText({
        'tips' : '请输入该管理员指令的处理函数名称 (ACMD_FUNC 部分的函数名)',
        'lower': True
    })
    
    # --------
    
    samefunc = Inputer().requireBool({
        'tips' : '管理员指令是否与处理函数名称一致 (%s)?' % funcname,
        'default' : True
    })
    
    # --------
    
    cmdname = funcname
    if not samefunc:
        cmdname = Inputer().requireText({
            'tips' : '请输入该管理员指令的名称 (ACMD_DEF2 使用)',
            'lower' : True
        })
    
    # --------
    
    print('-' * 70)
    Message.ShowInfo('请确认建立的信息, 确认后将开始修改代码.')
    print('-' * 70)
    Message.ShowInfo('开关名称 : %s' % define)
    Message.ShowInfo('管理员指令处理函数名称 : %s' % funcname)
    Message.ShowInfo('管理员指令名称 : %s' % cmdname)
    print('-' * 70)
    print('\n')

    nextstep = Inputer().requireBool({
        'tips' : '请仔细阅读上述信息, 确认要开始写入操作么?',
        'default' : False
    })

    if not nextstep:
        Message.ShowStatus('终止写入操作, 程序终止')
        Common.exit_with_pause(-1)

    # --------

    Message.ShowStatus('开始将管理员指令信息写入到源代码...')
    
    options = {
        'define' : define,
        'funcname' : funcname,
        'cmdname' : cmdname
    }
    
    insert_atcmd(inject, options)
    
    Message.ShowStatus('已经成功写入到源代码, 请检查并完善必要的注释.')
    print('')
    print('=' * 70)
    print('')
    print('感谢您的使用, 管理员指令添加助手已经执行完毕'.center(48))
    print('')
    print('=' * 70)

def main():
    os.chdir(os.path.split(os.path.realpath(__file__))[0])

    Common.welcome('管理员指令添加助手')

    options = {
        'is_pro' : Common.is_commercial_ver(slndir_path),
        'source_dirs' : '../../src',
        'process_exts' : ['.hpp', '.cpp'],
        'mark_format' : r'// PYHELP - ATCMD - INSERT POINT - <Section (\d{1,2})>',
        'mark_enum': InjectPoint,
        'mark_configure' : [
            {
                'id' : InjectPoint.PANDAS_SWITCH_DEFINE,
                'desc' : 'pandas.hpp @ 宏定义',
                'pro_offset' : 10
            },
            {
                'id' : InjectPoint.ATCMD_FUNC,
                'desc' : 'atcommand.cpp @ ACMD_FUNC 指令实际代码'
            },
            {
                'id' : InjectPoint.ATCMD_DEF,
                'desc' : 'atcommand.cpp @ ACMD_DEF 指令导出'
            }
        ]
    }

    guide(Injecter(options))
    Common.exit_with_pause()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt as _err:
        pass
