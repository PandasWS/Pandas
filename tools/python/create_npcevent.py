'''
//===== Pandas Python Script =================================
//= NPC事件添加助手
//===== By: ================================================== 
//= Sola丶小克
//===== Current Version: ===================================== 
//= 1.0
//===== Description: ========================================= 
//= 此脚本用于快速建立一个新的NPC事件(NpcEvent)
//===== Additional Comments: ================================= 
//= 1.0 首个版本. [Sola丶小克]
//============================================================

// PYHELP - NPCEVENT - INSERT POINT - <Section 1 / 41>
pandas.hpp @ Filter 类型的宏定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 2>
npc.hpp @ npce_event 中 Filter 类型的 NPCF_XXX 常量定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 3>
npc.cpp @ npc_get_script_event_name 中 Filter 类型的变量和常量关联

// PYHELP - NPCEVENT - INSERT POINT - <Section 4>
script.hpp @ Script_Config 中 Filter 类型的 xxx_event_name 变量定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 5>
script.cpp @ Script_Config 中 Filter 类型的事件名称定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 6>
script_constants.hpp 中 Filter 类型的 NPCF_XXX 常量导出定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 20>
npc.cpp 中 Filter 类型的 NPCF_XXX 常量设置

//============================================================

// PYHELP - NPCEVENT - INSERT POINT - <Section 7 / 47>
pandas.hpp @ Event  类型的宏定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 8>
npc.hpp @ npce_event 中 Event  类型的 NPCE_XXX 常量定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 9>
npc.cpp @ npc_get_script_event_name 中 Event  类型的变量和常量关联

// PYHELP - NPCEVENT - INSERT POINT - <Section 10>
script.hpp @ Script_Config 中 Event  类型的 xxx_filter_name 变量定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 11>
script.cpp @ Script_Config 中 Event  类型的事件名称定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 12>
script_constants.hpp 中 Event  类型的 NPCE_XXX 常量导出定义

//============================================================

// PYHELP - NPCEVENT - INSERT POINT - <Section 13 / 53>
pandas.hpp @ Express 类型的宏定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 14>
npc.hpp @ npce_event 中 Express 类型的 NPCX_XXX 常量定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 15>
npc.cpp @ npc_get_script_event_name 中 Express 类型的变量和常量关联

// PYHELP - NPCEVENT - INSERT POINT - <Section 16>
script.hpp @ Script_Config 中 Express 类型的 xxx_express_name 变量定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 17>
script.cpp @ Script_Config 中 Express 类型的事件名称定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 18>
script_constants.hpp 中 Express 类型的 NPCX_XXX 常量导出定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 19>
npc.cpp 中 Express 类型的 NPCX_XXX 常量设置
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
    F_PANDAS_SWITCH_DEFINE = 1
    F_NPC_CONSTANT_DEFINE = 2
    F_NPC_GET_SCRIPT_EVENT = 3
    F_SCRIPT_EVENT_VAR_DEFINE = 4
    F_SCRIPT_EVNET_NAME_DEFINE = 5
    F_SCRIPT_CONSTANTS_EXPORT = 6
    F_NPC_FILTER_SETTING = 20

    E_PANDAS_SWITCH_DEFINE = 7
    E_NPC_CONSTANT_DEFINE = 8
    E_NPC_GET_SCRIPT_EVENT = 9
    E_SCRIPT_EVENT_VAR_DEFINE = 10
    E_SCRIPT_EVNET_NAME_DEFINE = 11
    E_SCRIPT_CONSTANTS_EXPORT = 12

    X_PANDAS_SWITCH_DEFINE = 13
    X_NPC_CONSTANT_DEFINE = 14
    X_NPC_GET_SCRIPT_EVENT = 15
    X_SCRIPT_EVENT_VAR_DEFINE = 16
    X_SCRIPT_EVNET_NAME_DEFINE = 17
    X_SCRIPT_CONSTANTS_EXPORT = 18
    X_NPC_EXPRESS_SETTING = 19

def insert_for_filter_npcevent(inject, options):
    define = options['define']
    constant = options['constant']
    eventname = options['eventname']
    eventvar = options['eventvar']
    eventdesc = options['eventdesc']

    # pandas.hpp @ Filter 类型的宏定义
    inject.insert(InjectPoint.F_PANDAS_SWITCH_DEFINE, [
        '',
        '\t\t// {desc} [维护者昵称]'.format(
            desc = eventdesc
        ),
        '\t\t// 事件类型: Filter / 事件名称: {name}'.format(
            name = eventname
        ),
        '\t\t// 常量名称: {const} / 变量名称: {var}'.format(
            const = constant, var = eventvar
        ),
        '\t\t#define {define}'.format(define = define)
    ])

    # npc.hpp @ npce_event 中 Filter 类型的 NPCF_XXX 常量定义
    inject.insert(InjectPoint.F_NPC_CONSTANT_DEFINE, [
        '',
        '#ifdef {define}'.format(define = define),
        '\t{const},\t// {var}\t// {name}\t\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

    # npc.cpp @ npc_get_script_event_name 中 Filter 类型的变量和常量关联
    inject.insert(InjectPoint.F_NPC_GET_SCRIPT_EVENT, [
        '',
        '#ifdef {define}'.format(define = define),
        '\tcase {const}:'.format(const = constant),
        '\t\treturn script_config.{var};\t// {name}\t\t// {desc}'.format(
            var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

    # script.hpp @ Script_Config 中 Filter 类型的 xxx_filter_name 变量定义
    inject.insert(InjectPoint.F_SCRIPT_EVENT_VAR_DEFINE, [
        '',
        '#ifdef {define}'.format(define = define),
        '\tconst char* {var};\t// {const}\t// {name}\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

    # script.cpp @ Script_Config 中 Filter 类型的事件名称定义
    inject.insert(InjectPoint.F_SCRIPT_EVNET_NAME_DEFINE, [
        '',
        '#ifdef {define}'.format(define = define),
        '\t"{name}",\t// {const}\t\t// {var}\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])
    
    # script_constants.hpp 中 Filter 类型的 NPCF_XXX 常量导出定义
    inject.insert(InjectPoint.F_SCRIPT_CONSTANTS_EXPORT, [
        '',
        '#ifdef {define}'.format(define = define),
        '\texport_constant({const});\t// {var}\t// {name}\t\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])
	
    # npc.cpp 中 Filter 类型的 NPCF_XXX 常量设置
    inject.insert(InjectPoint.F_NPC_FILTER_SETTING, [
        '',
        '#ifdef {define}'.format(define = define),
        '\t\t{const},\t// {var}\t// {name}\t\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

def insert_for_normal_npcevent(inject, options):
    define = options['define']
    constant = options['constant']
    eventname = options['eventname']
    eventvar = options['eventvar']
    eventdesc = options['eventdesc']

    # pandas.hpp @ Event  类型的宏定义
    inject.insert(InjectPoint.E_PANDAS_SWITCH_DEFINE, [
        '',
        '\t// {desc} [维护者昵称]'.format(
            desc = eventdesc
        ),
        '\t// 事件类型: Event / 事件名称: {name}'.format(
            name = eventname
        ),
        '\t// 常量名称: {const} / 变量名称: {var}'.format(
            const = constant, var = eventvar
        ),
        '\t#define {define}'.format(define = define)
    ])

    # npc.hpp @ npce_event 中 Event  类型的 NPCE_XXX 常量定义
    inject.insert(InjectPoint.E_NPC_CONSTANT_DEFINE, [
        '',
        '#ifdef {define}'.format(define = define),
        '\t{const},\t// {var}\t// {name}\t\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

    # npc.cpp @ npc_get_script_event_name 中 Event  类型的变量和常量关联
    inject.insert(InjectPoint.E_NPC_GET_SCRIPT_EVENT, [
        '',
        '#ifdef {define}'.format(define = define),
        '\tcase {const}:'.format(const = constant),
        '\t\treturn script_config.{var};\t// {name}\t\t// {desc}'.format(
            var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

    # script.hpp @ Script_Config 中 Event  类型的 xxx_event_name 变量定义
    inject.insert(InjectPoint.E_SCRIPT_EVENT_VAR_DEFINE, [
        '',
        '#ifdef {define}'.format(define = define),
        '\tconst char* {var};\t// {const}\t// {name}\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

    # script.cpp @ Script_Config 中 Event  类型的事件名称定义
    inject.insert(InjectPoint.E_SCRIPT_EVNET_NAME_DEFINE, [
        '',
        '#ifdef {define}'.format(define = define),
        '\t"{name}",\t// {const}\t\t// {var}\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

    # script_constants.hpp 中 Event  类型的 NPCE_XXX 常量导出定义
    inject.insert(InjectPoint.E_SCRIPT_CONSTANTS_EXPORT, [
        '',
        '#ifdef {define}'.format(define = define),
        '\texport_constant({const});\t// {var}\t// {name}\t\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

def insert_for_express_npcevent(inject, options):
    define = options['define']
    constant = options['constant']
    eventname = options['eventname']
    eventvar = options['eventvar']
    eventdesc = options['eventdesc']

    # pandas.hpp @ Express 类型的宏定义
    inject.insert(InjectPoint.X_PANDAS_SWITCH_DEFINE, [
        '',
        '\t\t// {desc} [维护者昵称]'.format(
            desc = eventdesc
        ),
        '\t\t// 事件类型: Express / 事件名称: {name}'.format(
            name = eventname
        ),
        '\t\t// 常量名称: {const} / 变量名称: {var}'.format(
            const = constant, var = eventvar
        ),
        '\t\t#define {define}'.format(define = define)
    ])

    # npc.hpp @ npce_event 中 Express 类型的 NPCX_XXX 常量定义
    inject.insert(InjectPoint.X_NPC_CONSTANT_DEFINE, [
        '',
        '#ifdef {define}'.format(define = define),
        '\t{const},\t// {var}\t// {name}\t\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

    # npc.cpp @ npc_get_script_event_name 中 Express 类型的变量和常量关联
    inject.insert(InjectPoint.X_NPC_GET_SCRIPT_EVENT, [
        '',
        '#ifdef {define}'.format(define = define),
        '\tcase {const}:'.format(const = constant),
        '\t\treturn script_config.{var};\t// {name}\t\t// {desc}'.format(
            var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

    # script.hpp @ Script_Config 中 Express 类型的 xxx_express_name 变量定义
    inject.insert(InjectPoint.X_SCRIPT_EVENT_VAR_DEFINE, [
        '',
        '#ifdef {define}'.format(define = define),
        '\tconst char* {var};\t// {const}\t// {name}\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

    # script.cpp @ Script_Config 中 Express 类型的事件名称定义
    inject.insert(InjectPoint.X_SCRIPT_EVNET_NAME_DEFINE, [
        '',
        '#ifdef {define}'.format(define = define),
        '\t"{name}",\t// {const}\t\t// {var}\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])
    
    # script_constants.hpp 中 Express 类型的 NPCX_XXX 常量导出定义
    inject.insert(InjectPoint.X_SCRIPT_CONSTANTS_EXPORT, [
        '',
        '#ifdef {define}'.format(define = define),
        '\texport_constant({const});\t// {var}\t// {name}\t\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])
    
    # npc.cpp 中 Express 类型的 NPCX_XXX 常量设置
    inject.insert(InjectPoint.X_NPC_EXPRESS_SETTING, [
        '',
        '#ifdef {define}'.format(define = define),
        '\t\t{const},\t// {var}\t// {name}\t\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

def guide(inject):
    eventlist = [
        {
            'name' : 'Event 类型的标准事件',
            'desc' : '0 - Event   标准事件, 不能被 processhalt 指令打断'
        },
        {
            'name' : 'Filter 类型的过滤事件',
            'desc' : '1 - Filter  过滤事件, 事件立刻执行, 可以被 processhalt 指令打断'
        },
        {
            'name' : 'Express 类型的特殊事件',
            'desc' : '2 - Express 实时事件, 事件立刻执行, 不能被 processhalt 指令打断'
        }
    ]

    # 0 为 Event 类型的事件 | 1 为 Filter 类型的事件 | 2 为 Express 类型的事件
    eventtype = Inputer().requireSelect({
        'prompt' : '想创建的 NPC 事件类型',
        'option_name' : '事件类型',
        'data' : eventlist
    })

    # --------

    constant_prefix = 'NPCE_'
    if eventtype == 1:
        constant_prefix = 'NPCF_'
    elif eventtype == 2:
        constant_prefix = 'NPCX_'

    constant = Inputer().requireText({
        'tips' : '请输入该 NPC 事件的 {prefix} 常量名称 (自动大写, {prefix}的末尾部分)'.format(
            prefix = constant_prefix
        ),
        'prefix' : constant_prefix,
        'upper' : True
    })

    if eventtype == 0:
        define = 'Pandas_NpcEvent_' + constant.replace(constant_prefix, '')
        eventvar = constant.replace(constant_prefix, '').lower() + '_event_name'
    elif eventtype == 1:
        define = 'Pandas_NpcFilter_' + constant.replace(constant_prefix, '')
        eventvar = constant.replace(constant_prefix, '').lower() + '_filter_name'
    elif eventtype == 2:
        define = 'Pandas_NpcExpress_' + constant.replace(constant_prefix, '')
        eventvar = constant.replace(constant_prefix, '').lower() + '_express_name'
    else:
        Message.ShowError('发现无效的事件类型, 请确认..')
        Common.exit_with_pause(-1)

    # --------

    eventname = Inputer().requireText({
        'tips' : '请输入该 NPC 事件的名称 (以 On 开头, 末尾应为 Event | Filter | Express)'
    })

    if not eventname.startswith('On'):
        Message.ShowError('无论是什么类型的事件, 事件名称必须以 On 开头 (严格区分大小写)')
        Common.exit_with_pause(-1)

    if eventtype == 0:
        if not eventname.endswith('Event'):
            Message.ShowError('Event 类型的事件, 事件名称必须以 Event 结尾 (严格区分大小写)')
            Common.exit_with_pause(-1)

    if eventtype == 1:
        if not eventname.endswith('Filter'):
            Message.ShowError('Filter 类型的事件, 事件名称必须以 Filter 结尾 (严格区分大小写)')
            Common.exit_with_pause(-1)

    if eventtype == 2:
        if not eventname.endswith('Express'):
            Message.ShowError('Express 类型的事件, 事件名称必须以 Express 结尾 (严格区分大小写)')
            Common.exit_with_pause(-1)

    # --------

    eventdesc = Inputer().requireText({
        'tips' : '请输入该 NPC 事件的简短说明 (如: 当玩家杀死 MVP 魔物时触发事件)'
    })

    # --------

    print('-' * 70)
    Message.ShowInfo('请确认建立的信息, 确认后将开始修改代码.')
    print('-' * 70)
    Message.ShowInfo('事件类型 : %s' % eventlist[eventtype]['name'])
    Message.ShowInfo('常量名称 : %s' % constant)
    Message.ShowInfo('事件名称 : %s' % eventname)
    Message.ShowInfo('事件说明 : %s' % eventdesc)
    print('')
    Message.ShowInfo('开关名称 : %s' % define)
    Message.ShowInfo('变量名称 : %s' % eventvar)
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

    Message.ShowStatus('开始将 NPC 事件写入到源代码...')

    options = {
        'define' : define,
        'constant' : constant,
        'eventname' : eventname,
        'eventvar' : eventvar,
        'eventdesc' : eventdesc
    }

    if eventtype == 0:
        insert_for_normal_npcevent(inject, options)
    elif eventtype == 1:
        insert_for_filter_npcevent(inject, options)
    elif eventtype == 2:
        insert_for_express_npcevent(inject, options)

    Message.ShowStatus('已经成功写入到源代码, 请检查并完善必要的注释.')
    print('')
    print('=' * 70)
    print('')
    print('感谢您的使用, NPC 事件添加助手已经执行完毕'.center(48))
    print('')
    print('=' * 70)

def main():
    os.chdir(os.path.split(os.path.realpath(__file__))[0])

    Common.welcome('NPC事件添加助手')

    options = {
        'is_pro' : Common.is_commercial_ver(slndir_path),
        'source_dirs' : '../../src',
        'process_exts' : ['.hpp', '.cpp'],
        'mark_format' : r'// PYHELP - NPCEVENT - INSERT POINT - <Section (\d{1,2})>',
        'mark_enum': InjectPoint,
        'mark_configure' : [
            {
                'id' : InjectPoint.F_PANDAS_SWITCH_DEFINE,
                'desc' : 'pandas.hpp @ Filter 类型的宏定义',
                'pro_offset' : 40
            },
            {
                'id' : InjectPoint.F_NPC_CONSTANT_DEFINE,
                'desc' : 'npc.hpp @ npce_event 中 Filter 类型的 NPCF_XXX 常量定义'
            },
            {
                'id' : InjectPoint.F_NPC_GET_SCRIPT_EVENT,
                'desc' : 'npc.cpp @ npc_get_script_event_name 中 Filter 类型的变量和常量关联'
            },
            {
                'id' : InjectPoint.F_SCRIPT_EVENT_VAR_DEFINE,
                'desc' : 'script.hpp @ Script_Config 中 Filter 类型的 xxx_event_name 变量定义'
            },
            {
                'id' : InjectPoint.F_SCRIPT_EVNET_NAME_DEFINE,
                'desc' : 'script.cpp @ Script_Config 中 Filter 类型的事件名称定义'
            },
            {
                'id' : InjectPoint.F_SCRIPT_CONSTANTS_EXPORT,
                'desc' : 'script_constants.hpp 中 Filter 类型的 NPCF_XXX 常量导出定义'
            },
            {
                'id' : InjectPoint.F_NPC_FILTER_SETTING,
                'desc' : 'npc.cpp 中 Filter 类型的 NPCF_XXX 常量设置'
            },

            {
                'id' : InjectPoint.E_PANDAS_SWITCH_DEFINE,
                'desc' : 'pandas.hpp @ Event  类型的宏定义',
                'pro_offset' : 40
            },
            {
                'id' : InjectPoint.E_NPC_CONSTANT_DEFINE,
                'desc' : 'npc.hpp @ npce_event 中 Event  类型的 NPCE_XXX 常量定义'
            },
            {
                'id' : InjectPoint.E_NPC_GET_SCRIPT_EVENT,
                'desc' : 'npc.cpp @ npc_get_script_event_name 中 Event  类型的变量和常量关联'
            },
            {
                'id' : InjectPoint.E_SCRIPT_EVENT_VAR_DEFINE,
                'desc' : 'script.hpp @ Script_Config 中 Event  类型的 xxx_filter_name 变量定义'
            },
            {
                'id' : InjectPoint.E_SCRIPT_EVNET_NAME_DEFINE,
                'desc' : 'script.cpp @ Script_Config 中 Event  类型的事件名称定义'
            },
            {
                'id' : InjectPoint.E_SCRIPT_CONSTANTS_EXPORT,
                'desc' : 'script_constants.hpp 中 Event  类型的 NPCE_XXX 常量导出定义'
            },

            {
                'id' : InjectPoint.X_PANDAS_SWITCH_DEFINE,
                'desc' : 'pandas.hpp @ Express 类型的宏定义',
                'pro_offset' : 40
            },
            {
                'id' : InjectPoint.X_NPC_CONSTANT_DEFINE,
                'desc' : 'npc.hpp @ npce_event 中 Express 类型的 NPCX_XXX 常量定义'
            },
            {
                'id' : InjectPoint.X_NPC_GET_SCRIPT_EVENT,
                'desc' : 'npc.cpp @ npc_get_script_event_name 中 Express 类型的变量和常量关联'
            },
            {
                'id' : InjectPoint.X_SCRIPT_EVENT_VAR_DEFINE,
                'desc' : 'script.hpp @ Script_Config 中 Express 类型的 xxx_express_name 变量定义'
            },
            {
                'id' : InjectPoint.X_SCRIPT_EVNET_NAME_DEFINE,
                'desc' : 'script.cpp @ Script_Config 中 Express 类型的事件名称定义'
            },
            {
                'id' : InjectPoint.X_SCRIPT_CONSTANTS_EXPORT,
                'desc' : 'script_constants.hpp 中 Express 类型的 NPCX_XXX 常量导出定义'
            },
            {
                'id' : InjectPoint.X_NPC_EXPRESS_SETTING,
                'desc' : 'npc.cpp 中 Express 类型的 NPCX_XXX 常量设置'
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
