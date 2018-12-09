'''
此脚本用于快速建立一个新的地图标记(Mapflag)

// PYHELP - MAPFLAG - INSERT POINT - <Section 1>
rathena.hpp @ 宏定义

// PYHELP - MAPFLAG - INSERT POINT - <Section 2>
map.hpp @ MF_XXX 常量定义

// PYHELP - MAPFLAG - INSERT POINT - <Section 3>
script_constants.hpp @ MF_XXX 常量导出

// PYHELP - MAPFLAG - INSERT POINT - <Section 4>
atcommand.cpp @ mapflag GM指令的赋值屏蔽处理

// PYHELP - MAPFLAG - INSERT POINT - <Section 5>
map.cpp @ map_getmapflag_sub 的读取标记处理

// PYHELP - MAPFLAG - INSERT POINT - <Section 6>
map.cpp @ map_setmapflag_sub 的设置标记处理

// PYHELP - MAPFLAG - INSERT POINT - <Section 7>
npc.cpp @ npc_parse_mapflag 可选的标记处理

// PYHELP - MAPFLAG - INSERT POINT - <Section 8>
map.hpp @ 可选的变量添加

// PYHELP - MAPFLAG - INSERT POINT - <Section 9>
atcommand.cpp @ ACMD_FUNC(mapinfo) 可选的地图信息输出

// PYHELP - MAPFLAG - INSERT POINT - <Section 10>
script.cpp @ setmapflag 可选的脚本设置标记参数处理代码 - no use now

// PYHELP - MAPFLAG - INSERT POINT - <Section 11>
script.cpp @ getmapflag 可选的脚本读取标记参数处理代码 - no use now
'''

# -*- coding: utf-8 -*-

import os

from libs import InjectMarkController, InputController
from libs import Common, Message

def insert_for_normal_mapflag(inject, options, special = True):
    define = options['define']
    constant = options['constant']

    # rathena.hpp @ 宏定义
    inject.insert(1, [
        '',
        '\t// 是否启用 %s 地图标记 [维护者昵称]' % constant.lower().replace('mf_', ''),
        '\t// TODO: 请在此填写此地图标记的说明',
        '\t#define %s' % define
    ])

    # map.hpp @ MF_XXX 常量定义
    inject.insert(2, [
        '#ifdef %s' % define,
        '\t%s,' % constant,
        '#endif // %s' % define
    ])

    # script_constants.hpp @ MF_XXX 常量导出
    inject.insert(3, [
        '#ifdef %s' % define,
        '\texport_constant(%s);' % constant,
        '#endif // %s' % define,
        ''
    ])

    # atcommand.cpp @ ACMD_FUNC(mapinfo) 可选的地图信息输出
    # 注意: 这里只对普通地图标记进行处理
    # 数值型和其他复杂赋值型的地图标记, 若有需要在 mapinfo 指令中展现得自己手动加代码
    if special:
        inject.insert(9, [
            '#ifdef %s' % define,
            '\tif (map_getmapflag(m_id, %s))' % constant,
            '\t\tstrcat(atcmd_output, " %s |");' % define.replace('rAthenaCN_MapFlag_', ''),
            '#endif // %s' % define
        ])

def insert_for_one_param_mapflag(inject, options):
    define = options['define']
    constant = options['constant']
    var_name_1 = options['var_name_1']
    zero_disable = options['zero_disable']

    insert_for_normal_mapflag(inject, options, False)

    # atcommand.cpp @ mapflag GM指令的赋值屏蔽处理
    # 按照目前 rAthena 的代码理解, 只有普通的开关型地图标记, 才能被 @mapflag 赋值
    # 其他的地图标记都会禁止使用 @mapflag
    inject.insert(4, [
        '#ifdef %s' % define,
        '\t\t\tdisabled_mf.insert(disabled_mf.begin(), %s);' % constant,
        '#endif // %s' % define,
        ''
    ])

    # map.hpp @ 可选的变量添加
    inject.insert(8, [
        '#ifdef %s' % define,
        '\tint %s;' % var_name_1,
        '#endif // %s' % define,
        ''
    ])

    # map.cpp @ map_getmapflag_sub 的读取标记处理
    if zero_disable:
        inject.insert(5, [
            '#ifdef %s' % define,
            '\t\tcase %s:' % constant,
            '\t\t\treturn util::umap_get(mapdata->flag, static_cast<int16>(mapflag), 0) ? mapdata->%s : 0;' % var_name_1,
            '#endif // %s' % define,
            ''
        ])
    else:
        # 当第一个数值参数为 0 的时候, 如果不当做 mapflag 被关闭的话
        # 那么这里什么都不用做, rAthena 的默认代码会正确返回地图标记目前的开关状态
        pass

    # map.cpp @ map_setmapflag_sub 的设置标记处理
    if zero_disable:
        inject.insert(6, [
            '#ifdef %s' % define,
            '\t\tcase %s:' % constant,
            '\t\t\tif (!status)',
            '\t\t\t\tmapdata->%s = 0;' % var_name_1,
            '\t\t\telse {',
            '\t\t\t\tnullpo_retr(false, args);',
            '\t\t\t\tmapdata->%s = args->flag_val;' % var_name_1,
            '\t\t\t\tif (!args->flag_val) status = false;',
            '\t\t\t}',
            '\t\t\tmapdata->flag[mapflag] = status;',
            '\t\t\tbreak;',
            '#endif // %s' % define
        ])
    else:
        inject.insert(6, [
            '#ifdef %s' % define,
            '\t\tcase %s:' % constant,
            '\t\t\tif (!status)',
            '\t\t\t\tmapdata->%s = 0;' % var_name_1,
            '\t\t\telse {',
            '\t\t\t\tnullpo_retr(false, args);',
            '\t\t\t\tmapdata->%s = args->flag_val;' % var_name_1,
            '\t\t\t}',
            '\t\t\tmapdata->flag[mapflag] = status;',
            '\t\t\tbreak;',
            '#endif // %s' % define
        ])

    # npc.cpp @ npc_parse_mapflag 可选的标记处理
    if zero_disable:
        inject.insert(7, [
            '#ifdef %s' % define,
            '\t\tcase %s: {' % constant,
            '\t\t\t// 若脚本中 mapflag 指定的数值无效或为 0, 则立刻关闭这个地图标记',
            '\t\t\tif (state) {',
            '\t\t\t\tunion u_mapflag_args args = {};',
            '',
            '\t\t\t\tif (sscanf(w4, "%11d", &args.flag_val) < 1 || !args.flag_val)',
            '\t\t\t\t\tmap_setmapflag(m, mapflag, false);',
            '\t\t\t\telse',
            '\t\t\t\t\tmap_setmapflag_sub(m, mapflag, true, &args);',
            '\t\t\t}',
            '\t\t\telse',
            '\t\t\t\tmap_setmapflag(m, mapflag, false);',
            '\t\t\tbreak;',
            '\t\t}',
            '#endif // %s' % define,
            ''
        ])
    else:
        inject.insert(7, [
            '#ifdef %s' % define,
            '\t\tcase %s: {' % constant,
            '\t\t\t// 若脚本中 mapflag 指定的第一个数值参数无效,',
            '\t\t\t// 那么将此参数的值设为 0, 但不会阻断此地图标记的开启或关闭操作',
            '\t\t\tunion u_mapflag_args args = {};',
            '',
            '\t\t\tif (sscanf(w4, "%11d", &args.flag_val) < 1)',
            '\t\t\t\targs.flag_val = 0;',
            '',
            '\t\t\tmap_setmapflag_sub(m, mapflag, state, &args);',
            '\t\t\tbreak;',
            '\t\t}',
            '#endif // %s' % define,
            ''
        ])

def welecome():
    print('=' * 70)
    print('')
    print('地图标记添加助手'.center(62))
    print('')
    print('=' * 70)
    print('')

    Message.ShowInfo('在使用此脚本之前, 建议确保 src 目录的工作区是干净的.')
    Message.ShowInfo('这样添加结果如果不符合预期, 可以轻松的利用 git 进行重置操作.')

def guide(inject):

    define = InputController().requireText({
        'tips' : '请输入该地图标记的宏定义开关名称 (rAthenaCN_MapFlag_的末尾部分)',
        'prefix' : 'rAthenaCN_MapFlag_',
        'upper' : False
    })

    # --------

    constant = InputController().requireText({
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
            'name' : '携带单个数值参数的地图标记',
            'desc' : '1 - 携带单个数值参数的地图标记, 可以记录数值变量 (例如 bexp 标记)'
        },
        {
            'name' : '复杂赋值标记',
            'desc' : '2 - 复杂的赋值标记, 只单纯添加宏定义和 MF 常量, 其他的手动添加'
        }
    ]

    # flagtype = 0  # 0 为普通开关 | 1 为数值开关
    flagtype = InputController().requireSelect({
        'name' : '想创建的地图标记类型',
        'data' : flaglist
    })

    # --------

    var_name_1 = None
    if flagtype == 1:
        var_name_1 = InputController().requireText({
            'tips' : '请输入用于记录"第一个数值参数"的 map_data 结构成员变量名',
            'prefix' : '',
            'upper' : False
        })

    # --------

    zero_disable = False
    if flagtype == 1:
        zero_disable = InputController().requireBool({
            'tips' : '当"第一个数值参数"的值为 0 时, 是否表示移除此地图标记?',
            'default' : False
        })

    # --------

    print('-' * 70)
    Message.ShowInfo('请确认建立的信息, 确认后将开始修改代码.')
    print('-' * 70)
    Message.ShowInfo('开关名称 : %s' % define)
    Message.ShowInfo('常量名称 : %s' % constant)
    Message.ShowInfo('标记类型 : %s' % flaglist[flagtype]['name'])
    print('')
    Message.ShowInfo('第一个数值参数名称 : %s' % var_name_1)
    Message.ShowInfo('第一个数值参数的值为 0 时, 是否禁用此标记 : %s' % zero_disable)
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

    Message.ShowStatus('开始将地图标记信息写入到源代码...')

    options = {
        'define' : define,
        'constant' : constant,
        'var_name_1' : var_name_1,
        'zero_disable' : zero_disable
    }

    if flagtype == 0:
        insert_for_normal_mapflag(inject, options)
    elif flagtype == 1:
        insert_for_one_param_mapflag(inject, options)

    Message.ShowStatus('已经成功写入到源代码, 请检查并完善必要的注释.')
    print('')
    print('=' * 70)
    print('')
    print('感谢您的使用, 地图标记添加助手已经执行完毕'.center(48))
    print('')
    print('=' * 70)

def main():
    os.chdir(os.path.split(os.path.realpath(__file__))[0])

    welecome()

    options = {
        'source_dirs' : '../../src',
        'process_exts' : ['.hpp', '.cpp'],
        'mark_format' : r'// PYHELP - MAPFLAG - INSERT POINT - <Section (\d{1,2})>',
        'mark_counts' : 11
    }

    guide(InjectMarkController(options))
    Common.exitWithPause()

if __name__ == '__main__':
    main()
