'''
//===== Pandas Python Script =================================
//= 地图标记添加助手
//===== By: ================================================== 
//= Sola丶小克
//===== Current Version: ===================================== 
//= 1.0
//===== Description: ========================================= 
//= 此脚本用于快速建立一个新的地图标记(Mapflag)
//===== Additional Comments: ================================= 
//= 1.0 首个版本. [Sola丶小克]
//============================================================

// PYHELP - MAPFLAG - INSERT POINT - <Section 1 / 11>
pandas.hpp @ 宏定义

// PYHELP - MAPFLAG - INSERT POINT - <Section 2>
map.hpp @ MF_XXX 常量定义

// PYHELP - MAPFLAG - INSERT POINT - <Section 3>
script_constants.hpp @ MF_XXX 常量导出

// PYHELP - MAPFLAG - INSERT POINT - <Section 4>
map.cpp @ map_flags_init 地图标记配置
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
    MAP_MAPFLAG_DEFINE = 2
    SCRIPT_CONSTANTS_EXPORT = 3
    MAPFLAG_CONFIGURE = 4

def insert_mapflag(inject, options):
    flagtype = options['flagtype']
    define = options['define']
    constant = options['constant']
    args_count = options['args_count']
    default_disable = options['default_disable']
    atcommand_disable = options['atcommand_disable']

    # pandas.hpp @ 宏定义
    inject.insert(InjectPoint.PANDAS_SWITCH_DEFINE, [
        '',
        '\t// 是否启用 %s 地图标记 [维护者昵称]' % constant.lower().replace('mf_', ''),
        '\t// TODO: 请在此填写此地图标记的说明',
        '\t#define %s' % define
    ])

    # map.hpp @ MF_XXX 常量定义
    inject.insert(InjectPoint.MAP_MAPFLAG_DEFINE, [
        '#ifdef %s' % define,
        '\t%s,' % constant,
        '#endif // %s' % define
    ])

    # script_constants.hpp @ MF_XXX 常量导出
    inject.insert(InjectPoint.SCRIPT_CONSTANTS_EXPORT, [
        '#ifdef %s' % define,
        '\texport_constant(%s);' % constant,
        '#endif // %s' % define,
        ''
    ])

    # map.cpp @ map_flags_init 地图标记配置
    
    if flagtype == 0:
        inject.insert(InjectPoint.MAPFLAG_CONFIGURE, [
            '#ifdef %s' % define,
            '\tmapflag_config.insert(std::make_pair(%s, s_mapflag_item{' % constant,
            '\t\t/* 地图标记名称 (主要用在 @mapinfo 指令中显示) */ "%s",' % define.replace('Pandas_MapFlag_', ''),
            '\t\t/* 当有参数值的时候, 若全部参数的值等于默认值时, 是否自动关闭此地图标记 */ %s,' % ('true' if default_disable else 'false'),
            '\t\t/* 禁止在 @mapflag 指令中开启此地图标记 */ %s,' % ('true' if atcommand_disable else 'false'),
            '\t\t/* 参数列表定义(支持多参数), 格式: {默认值, 最小值, 最大值, <"可选: 参数单位">} */ {}',
            '\t}));',
            '#endif // %s' % define,
            ''
        ])
    else:
        # 根据 args_count 的数量构建一个 list,
        # 其中每一个 item 的内容固定为字符串: "{0, 0, INT_MAX}",
        # 重复的次数为 args_count 的数量
        args_list = ['\t\t\t{0, 0, INT_MAX}'] * args_count
        
        inject.insert(InjectPoint.MAPFLAG_CONFIGURE, [
            '#ifdef %s' % define,
            '\tmapflag_config.insert(std::make_pair(%s, s_mapflag_item{' % constant,
            '\t\t/* 地图标记名称 (主要用在 @mapinfo 指令中显示) */ "%s",' % define.replace('Pandas_MapFlag_', ''),
            '\t\t/* 当有参数值的时候, 若全部参数的值等于默认值时, 是否自动关闭此地图标记 */ %s,' % ('true' if default_disable else 'false'),
            '\t\t/* 禁止在 @mapflag 指令中开启此地图标记 */ %s,' % ('true' if atcommand_disable else 'false'),
            '\t\t/* 参数列表定义(支持多参数), 格式: {默认值, 最小值, 最大值, <"可选: 参数单位">} */ {',
            ',\n'.join(args_list),
            '\t\t}',
            '\t}));',
            '#endif // %s' % define,
            ''
        ])

def guide(inject):

    define = Inputer().requireText({
        'tips' : '请输入该地图标记的宏定义开关名称 (Pandas_MapFlag_的末尾部分)',
        'prefix' : 'Pandas_MapFlag_'
    })

    # --------

    constant = Inputer().requireText({
        'tips' : '请输入该地图标记的 MF 常量名称 (自动大写, MF_的末尾部分)',
        'prefix' : 'MF_',
        'upper' : True
    })

    # --------

    flaglist = [
        {
            'name' : '普通开关式的地图标记',
            'desc' : '0 - 普通的地图标记开关, 只有两个状态(开/关)'
        },
        {
            'name' : '携带数值参数的地图标记',
            'desc' : '1 - 携带数值参数的地图标记, 可以记录数值变量 (例如 bexp 标记)'
        }
    ]

    # flagtype = 0  # 0 为普通开关 | 1 为数值开关
    flagtype = Inputer().requireSelect({
        'prompt' : '想创建的地图标记类型',
        'option_name' : '地图标记类型',
        'data' : flaglist
    })

    # --------
    
    args_count = 0
    if flagtype == 1:
        args_count = Inputer().requireInt({
            'tips' : '请输入该地图标记的"数值参数"数量 (有效值: 1-4)',
			'allow_empty' : False
        })
    
        if args_count < 1 or args_count > 4:
            Message.ShowError('无效的参数数量, 目前只支持 1-4 个数值参数, 请重新运行程序')
            Common.exit_with_pause(-1)

    # --------

    default_disable = False
    if flagtype == 1:
        default_disable = Inputer().requireBool({
            'tips' : '当"全部数值参数"的值等于默认值时, 是否移除此地图标记?',
            'default' : False
        })

    # --------

    atcommand_disable = False
    if flagtype == 1:
        atcommand_disable = Inputer().requireBool({
            'tips' : '禁止在 @mapflag 指令中调整此地图标记?',
            'default' : False
        })

    # --------

    print('-' * 70)
    Message.ShowInfo('请确认建立的信息, 确认后将开始修改代码.')
    print('-' * 70)
    Message.ShowInfo('开关名称 : %s' % define)
    Message.ShowInfo('常量名称 : %s' % constant)
    Message.ShowInfo('标记类型 : %s' % flaglist[flagtype]['name'])
    if flagtype == 1:
        print('')
        Message.ShowInfo('该地图标记支持的数值参数数量 : %d 个' % args_count)
        Message.ShowInfo('全部参数等于默认值时, 自动移除地图标记 : %s' % default_disable)
        Message.ShowInfo('禁止在 @mapflag 指令中调整此地图标记 : %s' % atcommand_disable)
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

    Message.ShowStatus('开始将地图标记信息写入到源代码...')

    options = {
        'flagtype' : flagtype,
        'define' : define,
        'constant' : constant,
        'args_count' : args_count,
        'default_disable' : default_disable,
        'atcommand_disable' : atcommand_disable
    }

    insert_mapflag(inject, options)

    Message.ShowStatus('已经成功写入到源代码, 请检查并完善必要的注释.')
    print('')
    print('=' * 70)
    print('')
    print('感谢您的使用, 地图标记添加助手已经执行完毕'.center(48))
    print('')
    print('=' * 70)

def main():
    os.chdir(os.path.split(os.path.realpath(__file__))[0])

    Common.welcome('地图标记添加助手')

    options = {
        'is_pro' : Common.is_commercial_ver(slndir_path),
        'source_dirs' : '../../src',
        'process_exts' : ['.hpp', '.cpp'],
        'mark_format' : r'// PYHELP - MAPFLAG - INSERT POINT - <Section (\d{1,2})>',
        'mark_enum': InjectPoint,
        'mark_configure' : [
            {
                'id' : InjectPoint.PANDAS_SWITCH_DEFINE,
                'desc' : 'pandas.hpp @ 宏定义',
                'pro_offset' : 10
            },
            {
                'id' : InjectPoint.MAP_MAPFLAG_DEFINE,
                'desc' : 'map.hpp @ MF_XXX 常量定义'
            },
            {
                'id' : InjectPoint.SCRIPT_CONSTANTS_EXPORT,
                'desc' : 'script_constants.hpp @ MF_XXX 常量导出'
            },
            {
                'id' : InjectPoint.MAPFLAG_CONFIGURE,
                'desc' : 'map.cpp @ map_flags_init 地图标记配置'
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
