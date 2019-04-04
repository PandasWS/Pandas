'''
//===== Pandas Python Script ============================== 
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

// PYHELP - NPCEVENT - INSERT POINT - <Section 1>
pandas.hpp @ Filter 类型的宏定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 2>
pandas.hpp @ Event  类型的宏定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 3>
npc.hpp @ npce_event 中 Filter 类型的 NPCE_XXX 常量定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 4>
npc.hpp @ npce_event 中 Event  类型的 NPCE_XXX 常量定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 5>
npc.cpp @ npc_get_script_event_name 中 Filter 类型的变量和常量关联

// PYHELP - NPCEVENT - INSERT POINT - <Section 6>
npc.cpp @ npc_get_script_event_name 中 Event  类型的变量和常量关联

// PYHELP - NPCEVENT - INSERT POINT - <Section 7>
script.hpp @ Script_Config 中 Filter 类型的 xxx_event_name 变量定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 8>
script.hpp @ Script_Config 中 Event  类型的 xxx_event_name 变量定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 9>
script.cpp @ Script_Config 中 Filter 类型的事件名称定义

// PYHELP - NPCEVENT - INSERT POINT - <Section 10>
script.cpp @ Script_Config 中 Event  类型的事件名称定义
'''

# -*- coding: utf-8 -*-

import os

from libs import InjectController, InputController
from libs import Common, Message


def insert_for_normal_npcevent(inject, options):
    define = options['define']
    constant = options['constant']
    eventname = options['eventname']
    eventvar = options['eventvar']
    eventdesc = options['eventdesc']

    # pandas.hpp @ Event  类型的宏定义
    inject.insert(2, [
        '',
        '\t// {desc} - {name} [维护者昵称]'.format(
            desc = eventdesc, name = eventname
        ),
        '\t// 类型: Event 类型 / 常量名称: {const} / 变量名称: {var}'.format(
            const = constant, var = eventvar
        ),
        '\t#define {define}'.format(define = define)
    ])

    # npc.hpp @ npce_event 中 Event  类型的 NPCE_XXX 常量定义
    inject.insert(4, [
        '',
        '#ifdef {define}'.format(define = define),
        '\t{const},\t// {var}\t// {name}\t\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

    # npc.cpp @ npc_get_script_event_name 中 Event  类型的变量和常量关联
    inject.insert(6, [
        '',
        '#ifdef {define}'.format(define = define),
        '\tcase {const}:'.format(const = constant),
        '\t\treturn script_config.{var};\t// {name}\t\t// {desc}'.format(
            var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

    # script.hpp @ Script_Config 中 Event  类型的 xxx_event_name 变量定义
    inject.insert(8, [
        '',
        '#ifdef {define}'.format(define = define),
        '\tconst char* {var};\t// {const}\t// {name}\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

    # script.cpp @ Script_Config 中 Event  类型的事件名称定义
    inject.insert(10, [
        '',
        '#ifdef {define}'.format(define = define),
        '\t"{name}",\t// {const}\t\t// {var}\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

def insert_for_filter_npcevent(inject, options):
    define = options['define']
    constant = options['constant']
    eventname = options['eventname']
    eventvar = options['eventvar']
    eventdesc = options['eventdesc']

    # pandas.hpp @ Filter 类型的宏定义
    inject.insert(1, [
        '',
        '\t\t// {desc} - {name} [维护者昵称]'.format(
            desc = eventdesc, name = eventname
        ),
        '\t\t// 类型: Filter 类型 / 常量名称: {const} / 变量名称: {var}'.format(
            const = constant, var = eventvar
        ),
        '\t\t#define {define}'.format(define = define)
    ])

    # npc.hpp @ npce_event 中 Filter 类型的 NPCE_XXX 常量定义
    inject.insert(3, [
        '',
        '#ifdef {define}'.format(define = define),
        '\t{const},\t// {var}\t// {name}\t\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

    # npc.cpp @ npc_get_script_event_name 中 Filter 类型的变量和常量关联
    inject.insert(5, [
        '',
        '#ifdef {define}'.format(define = define),
        '\tcase {const}:'.format(const = constant),
        '\t\treturn script_config.{var};\t// {name}\t\t// {desc}'.format(
            var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

    # script.hpp @ Script_Config 中 Filter 类型的 xxx_event_name 变量定义
    inject.insert(7, [
        '',
        '#ifdef {define}'.format(define = define),
        '\tconst char* {var};\t// {const}\t// {name}\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

    # script.cpp @ Script_Config 中 Filter 类型的事件名称定义
    inject.insert(9, [
        '',
        '#ifdef {define}'.format(define = define),
        '\t"{name}",\t// {const}\t\t// {var}\t// {desc}'.format(
            const = constant, var = eventvar, name = eventname, desc = eventdesc
        ),
        '#endif // {define}'.format(define = define)
    ])

def welecome():
    print('=' * 70)
    print('')
    print('NPC 事件添加助手'.center(62))
    print('')
    print('=' * 70)
    print('')

    Message.ShowInfo('在使用此脚本之前, 建议确保 src 目录的工作区是干净的.')
    Message.ShowInfo('这样添加结果如果不符合预期, 可以轻松的利用 git 进行重置操作.')

def guide(inject):
    eventlist = [
        {
            'name' : 'Event 类型的标准事件',
            'desc' : '0 - Event  类型的标准事件, 不能被 processhalt 指令打断'
        },
        {
            'name' : 'Filter 类型的过滤事件',
            'desc' : '1 - Filter 类型的过滤事件, 可以被 processhalt 指令打断'
        }
    ]

    # 0 为 Event 类型的事件 | 1 为 Filter 类型的事件
    eventtype = InputController().requireSelect({
        'name' : '想创建的 NPC 事件类型',
        'data' : eventlist
    })

    # --------

    constant_prefix = 'NPCE_' if eventtype == 0 else 'NPCF_'
    constant = InputController().requireText({
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
    else:
        Message.ShowError('发现无效的事件类型, 请确认..')
        Common.exitWithPause(-1)

    # --------

    eventname = InputController().requireText({
        'tips' : '请输入该 NPC 事件的名称 (以 On 开头, 末尾应为 Event 或 Filter)',
        'prefix' : ''
    })

    if not eventname.startswith('On'):
        Message.ShowError('无论是什么类型的事件, 事件名称必须以 On 开头 (严格区分大小写)')
        Common.exitWithPause(-1)

    if eventtype == 0:
        if not eventname.endswith('Event'):
            Message.ShowError('Event 类型的事件, 事件名称必须以 Event 结尾 (严格区分大小写)')
            Common.exitWithPause(-1)

    if eventtype == 1:
        if not eventname.endswith('Filter'):
            Message.ShowError('Filter 类型的事件, 事件名称必须以 Filter 结尾 (严格区分大小写)')
            Common.exitWithPause(-1)

    # --------

    eventdesc = InputController().requireText({
        'tips' : '请输入该 NPC 事件的简短说明 (如: 当玩家杀死 MVP 魔物时触发事件)',
        'prefix' : ''
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

    nextstep = InputController().requireBool({
        'tips' : '请仔细阅读上述信息, 确认要开始写入操作么?',
        'default' : False
    })

    if not nextstep:
        Message.ShowStatus('终止写入操作, 程序终止')
        Common.exitWithPause(-1)

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

    Message.ShowStatus('已经成功写入到源代码, 请检查并完善必要的注释.')
    print('')
    print('=' * 70)
    print('')
    print('感谢您的使用, NPC 事件添加助手已经执行完毕'.center(48))
    print('')
    print('=' * 70)

def main():
    os.chdir(os.path.split(os.path.realpath(__file__))[0])

    welecome()

    options = {
        'source_dirs' : '../../src',
        'process_exts' : ['.hpp', '.cpp'],
        'mark_format' : r'// PYHELP - NPCEVENT - INSERT POINT - <Section (\d{1,2})>',
        'mark_counts' : 10
    }

    guide(InjectController(options))
    Common.exitWithPause()

if __name__ == '__main__':
    main()
