'''
//===== Pandas Python Script =================================
//= 脚本指令添加助手
//===== By: ================================================== 
//= Sola丶小克
//===== Current Version: ===================================== 
//= 1.0
//===== Description: ========================================= 
//= 此脚本用于快速建立一个新的脚本指令(Script Command)
//===== Additional Comments: ================================= 
//= 1.0 首个版本. [Sola丶小克]
//============================================================

// PYHELP - SCRIPTCMD - INSERT POINT - <Section 1 / 11>
pandas.hpp @ 宏定义

// PYHELP - SCRIPTCMD - INSERT POINT - <Section 2>
script.cpp @ BUILDIN_FUNC 脚本指令实际代码

// PYHELP - SCRIPTCMD - INSERT POINT - <Section 3>
script.cpp @ BUILDIN_DEF 脚本指令导出
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
    SCRIPT_BUILDIN_FUNC = 2
    SCRIPT_BUILDIN_DEF = 3

def insert_scriptcmd(inject, options):
    define = options['define']
    funcname = options['funcname']
    cmdname = options['cmdname']
    argsmode = options['argsmode']
    
    # pandas.hpp @ 宏定义
    inject.insert(InjectPoint.PANDAS_SWITCH_DEFINE, [
        '',
        '\t// 是否启用 %s 脚本指令 [维护者昵称]' % cmdname,
        '\t// TODO: 请在此填写此脚本指令的说明',
        '\t#define %s' % define
    ])
    
    usage = ' * 用法: %s;' % cmdname
    if argsmode != '':
        usage = ' * 用法: %s <请补充完整参数说明>;' % cmdname
    
    # script.cpp @ BUILDIN_FUNC 脚本指令实际代码
    inject.insert(InjectPoint.SCRIPT_BUILDIN_FUNC, [
        '#ifdef %s' % define,
        '/* ===========================================================',
        ' * 指令: %s' % cmdname,
        ' * 描述: 请在此补充该脚本指令的说明',
        usage,
        ' * 返回: 请说明返回值',
        ' * 作者: 维护者昵称',
        ' * -----------------------------------------------------------*/',
        'BUILDIN_FUNC(%s) {' % funcname,
        '\t// TODO: 请在此填写脚本指令的实现代码',
        '\treturn SCRIPT_CMD_SUCCESS;',
        '}',
        '#endif // %s' % define,
        ''
    ])
    
    # script.cpp @ BUILDIN_DEF 脚本指令导出
    
    if funcname == cmdname:
        defcontent = '\tBUILDIN_DEF(%s,"%s"),\t\t\t\t\t\t// 在此写上脚本指令说明 [维护者昵称]' % (funcname, argsmode)
    else:
        defcontent = '\tBUILDIN_DEF2(%s,"%s","%s"),\t\t\t\t\t\t// 在此写上脚本指令说明 [维护者昵称]' % (funcname, cmdname, argsmode)
    
    inject.insert(InjectPoint.SCRIPT_BUILDIN_DEF, [
        '#ifdef %s' % define,
        defcontent,
        '#endif // %s' % define
    ])

def guide(inject):

    define = Inputer().requireText({
        'tips' : '请输入该脚本指令的宏定义开关名称 (Pandas_ScriptCommand_的末尾部分)',
        'prefix' : 'Pandas_ScriptCommand_'
    })
    
    # --------

    funcname = Inputer().requireText({
        'tips' : '请输入该脚本指令的处理函数名称 (BUILDIN_FUNC 部分的函数名)',
        'lower': True
    })
    
    # --------
    
    samefunc = Inputer().requireBool({
        'tips' : '脚本指令是否与处理函数名称一致 (%s)?' % funcname,
        'default' : True
    })
    
    # --------
    
    cmdname = funcname
    if not samefunc:
        cmdname = Inputer().requireText({
            'tips' : '请输入该脚本指令的名称 (BUILDIN_DEF2 使用)',
            'lower' : True
        })
    
    # --------
    
    argsmode = Inputer().requireText({
        'tips' : r'请输入该脚本指令的参数模式 (如一个或多个的: i\s\? 为空则直接回车)',
        'lower' : True,
        'allow_empty' : True
    })
    
    # --------
    
    print('-' * 70)
    Message.ShowInfo('请确认建立的信息, 确认后将开始修改代码.')
    print('-' * 70)
    Message.ShowInfo('开关名称 : %s' % define)
    Message.ShowInfo('脚本处理函数名称 : %s' % funcname)
    Message.ShowInfo('脚本指令名称 : %s' % cmdname)
    Message.ShowInfo('脚本指令的参数模式 : %s' % argsmode)
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

    Message.ShowStatus('开始将脚本指令信息写入到源代码...')
    
    options = {
        'define' : define,
        'funcname' : funcname,
        'cmdname' : cmdname,
        'argsmode' : argsmode
    }
    
    insert_scriptcmd(inject, options)
    
    Message.ShowStatus('已经成功写入到源代码, 请检查并完善必要的注释.')
    print('')
    print('=' * 70)
    print('')
    print('感谢您的使用, 脚本指令添加助手已经执行完毕'.center(48))
    print('')
    print('=' * 70)

def main():
    os.chdir(os.path.split(os.path.realpath(__file__))[0])

    Common.welcome('脚本指令添加助手')

    options = {
        'is_pro' : Common.is_commercial_ver(slndir_path),
        'source_dirs' : '../../src',
        'process_exts' : ['.hpp', '.cpp'],
        'mark_format' : r'// PYHELP - SCRIPTCMD - INSERT POINT - <Section (\d{1,2})>',
        'mark_enum': InjectPoint,
        'mark_configure' : [
            {
                'id' : InjectPoint.PANDAS_SWITCH_DEFINE,
                'desc' : 'pandas.hpp @ 宏定义',
                'pro_offset' : 10
            },
            {
                'id' : InjectPoint.SCRIPT_BUILDIN_FUNC,
                'desc' : 'script.cpp @ BUILDIN_FUNC 脚本指令实际代码'
            },
            {
                'id' : InjectPoint.SCRIPT_BUILDIN_DEF,
                'desc' : 'script.cpp @ BUILDIN_DEF 脚本指令导出'
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
