'''
//===== Pandas Python Script =================================
//= 战斗配置选项添加助手
//===== By: ================================================== 
//= Sola丶小克
//===== Current Version: ===================================== 
//= 1.0
//===== Description: ========================================= 
//= 此脚本用于快速建立一个新的战斗配置选项
//===== Additional Comments: ================================= 
//= 1.0 首个版本. [Sola丶小克]
//============================================================

// PYHELP - BATTLECONFIG - INSERT POINT - <Section 1 / 11>
pandas.hpp @ 宏定义

// PYHELP - BATTLECONFIG - INSERT POINT - <Section 2>
battle.hpp @ 战斗配置选项的变量声明

// PYHELP - BATTLECONFIG - INSERT POINT - <Section 3>
battle.cpp @ 战斗配置选项的关联和默认值配置

// PYHELP - BATTLECONFIG - INSERT POINT - <Section 4 / 14>
pandas.conf @ 战斗配置选项的默认添加位置
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
    BATTLE_VAR_DEFINE = 2
    BATTLE_DEFAULT_DEFINE = 3
    PANDAS_CONF_INSERT = 4

def insert_battle_config(inject, options):
    define = options['define']
    option_name = options['option_name']
    var_name = options['var_name']
    def_val = options['def_val']
    min_val = options['min_val']
    max_val = options['max_val']
    
    # pandas.hpp @ 宏定义
    inject.insert(InjectPoint.PANDAS_SWITCH_DEFINE, [
        '',
        '\t// 是否启用 %s 配置选项及其功能 [维护者昵称]' % option_name,
        '\t// TODO: 请在此填写战斗配置选项的功能说明',
        '\t#define %s' % define
    ])
    
    # battle.hpp @ 战斗配置选项的变量声明
    inject.insert(InjectPoint.BATTLE_VAR_DEFINE, [
        '#ifdef %s' % define,
        '\tint %s;' % var_name,
        '#endif // %s' % define
    ])
    
    # battle.cpp @ 战斗配置选项的关联和默认值配置
    inject.insert(InjectPoint.BATTLE_DEFAULT_DEFINE, [
        '#ifdef %s' % define,
        '\t{ "%s",    &battle_config.%s,    %s,    %s,    %s,    },' % (option_name, var_name, def_val, min_val, max_val),
        '#endif // %s' % define
    ])
    
    # pandas.conf @ 战斗配置选项的默认添加位置
    inject.insert(InjectPoint.PANDAS_CONF_INSERT, [
        '// 请在此填写战斗配置选项的功能说明',
        '%s: %s' % (option_name, def_val),
        ''
    ])

def guide(inject):

    define = Inputer().requireText({
        'tips' : '请输入该战斗配置选项的宏定义开关名称 (Pandas_BattleConfig_的末尾部分)',
        'prefix' : 'Pandas_BattleConfig_'
    })
    
    # --------
    
    option_name = Inputer().requireText({
        'tips' : '请输入该战斗配置选项的选项名称'
    })
    
    # --------
    
    var_name = Inputer().requireText({
        'tips' : '请输入该战斗配置选项的变量名称 (不填默认与选项名称一致)',
        'default' : option_name.replace('.', '_')
    })
    
    # --------
    
    def_val = Inputer().requireInt({
        'tips' : '请输入默认值 (不填则默认为 0)',
		'allow_empty' : True
    })
    
    # --------
    
    min_val = Inputer().requireInt({
        'tips' : '请输入允许设置的最小值 (不填则默认为 0)',
		'allow_empty' : True
    })
    
    # --------
    
    max_val = Inputer().requireInt({
        'tips' : '请输入允许设置的最大值 (不填则默认为 INT_MAX)',
		'allow_empty' : True
    })
    
    if max_val == 0:
        max_val = 2147483647
    
    if max_val <= min_val:
        Message.ShowError('最大值比最小值还要小, 这不合理...')
        Common.exit_with_pause(-1)
    
    def_val = str(def_val)
    min_val = str(min_val)
    max_val = 'INT_MAX' if max_val == 2147483647 else str(max_val)
    
    # --------
    
    print('-' * 70)
    Message.ShowInfo('请确认建立的信息, 确认后将开始修改代码.')
    print('-' * 70)
    Message.ShowInfo('开关名称 : %s' % define)
    Message.ShowInfo('选项名称 : %s' % option_name)
    Message.ShowInfo('变量名称 : %s' % var_name)
    print('')
    Message.ShowInfo('选项默认值 : %s' % def_val)
    Message.ShowInfo('选项最小值 : %s' % min_val)
    Message.ShowInfo('选项最大值 : %s' % max_val)
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
    
    Message.ShowStatus('开始将战斗配置选项写入到源代码...')
    
    options = {
        'define' : define,
        'option_name' : option_name,
        'var_name' : var_name,
        'def_val' : def_val,
        'min_val' : min_val,
        'max_val' : max_val
    }
    
    insert_battle_config(inject, options)
    
    Message.ShowStatus('已经成功写入到源代码, 请检查并完善必要的注释.')
    print('')
    print('=' * 70)
    print('')
    print('感谢您的使用, 战斗配置选项添加助手已经执行完毕'.center(50))
    print('')
    print('=' * 70)

def main():
    os.chdir(os.path.split(os.path.realpath(__file__))[0])

    Common.welcome('战斗配置选项添加助手')

    options = {
        'is_pro' : Common.is_commercial_ver(slndir_path),
        'source_dirs' : ['../../src', '../../conf/battle'],
        'process_exts' : ['.hpp', '.cpp', '.conf'],
        'mark_format' : r'// PYHELP - BATTLECONFIG - INSERT POINT - <Section (\d{1,2})>',
        'mark_enum': InjectPoint,
        'mark_configure' : [
            {
                'id' : InjectPoint.PANDAS_SWITCH_DEFINE,
                'desc' : 'pandas.hpp @ 宏定义',
                'pro_offset' : 10
            },
            {
                'id' : InjectPoint.BATTLE_VAR_DEFINE,
                'desc' : 'battle.hpp @ 战斗配置选项的变量声明'
            },
            {
                'id' : InjectPoint.BATTLE_DEFAULT_DEFINE,
                'desc' : 'battle.cpp @ 战斗配置选项的关联和默认值配置'
            },
            {
                'id' : InjectPoint.PANDAS_CONF_INSERT,
                'desc' : 'pandas.conf @ 战斗配置选项的默认添加位置',
                'pro_offset' : 10
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
