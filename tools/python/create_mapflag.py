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
atcommand.cpp @ mapflag GM指令的赋值屏蔽处理

// PYHELP - MAPFLAG - INSERT POINT - <Section 5>
map.cpp @ map_getmapflag_sub 的读取标记处理

// PYHELP - MAPFLAG - INSERT POINT - <Section 6>
map.cpp @ map_setmapflag_sub 的设置标记处理

// PYHELP - MAPFLAG - INSERT POINT - <Section 7>
npc.cpp @ npc_parse_mapflag 可选的标记处理

// PYHELP - MAPFLAG - INSERT POINT - <Section 8>
atcommand.cpp @ ACMD_FUNC(mapinfo) 可选的地图信息输出

// PYHELP - MAPFLAG - INSERT POINT - <Section 9>
script.cpp @ setmapflag 可选的脚本设置标记参数处理代码 - no use now

// PYHELP - MAPFLAG - INSERT POINT - <Section 10>
script.cpp @ getmapflag 可选的脚本读取标记参数处理代码 - no use now
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
    ATCOMMAND_MAPFLAG_BLOCK = 4
    MAP_GETMAPFLAG_SUB = 5
    MAP_SETMAPFLAG_SUB = 6
    NPC_PARSE_MAPFLAG = 7
    ATCOMMAND_MAPINFO = 8
    SCRIPT_SETMAPFLAG = 9
    SCRIPT_GETMAPFLAG = 10

def insert_for_normal_mapflag(inject, options):
    flagtype = options['flagtype']
    define = options['define']
    constant = options['constant']
    default_val = options['default_val']

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

    # atcommand.cpp @ ACMD_FUNC(mapinfo) 可选的地图信息输出
    if flagtype == 0:
        inject.insert(InjectPoint.ATCOMMAND_MAPINFO, [
            '#ifdef %s' % define,
            '\tif (map_getmapflag(m_id, %s))' % constant,
            '\t\tstrcat(atcmd_output, " %s |");' % define.replace('Pandas_MapFlag_', ''),
            '#endif // %s' % define
        ])
    elif flagtype == 1:
        inject.insert(InjectPoint.ATCOMMAND_MAPINFO, [
            '#ifdef %s' % define,
            '\tif (map_getmapflag(m_id, %s)) {' % constant,
            '\t\tchar mes[256] = { 0 };',
            '\t\tsnprintf(mes, sizeof(mes), " {shortname}: %d |", map_getmapflag_param(m_id, {constant}, {default_val}));'.format(
                shortname = define.replace('Pandas_MapFlag_', ''), constant = constant, default_val = default_val
            ),
            '\t\tstrcat(atcmd_output, mes);',
            '\t}',
            '#endif // %s' % define
        ])

def insert_for_one_param_mapflag(inject, options):
    define = options['define']
    constant = options['constant']
    default_val = options['default_val']
    default_disable = options['default_disable']

    insert_for_normal_mapflag(inject, options)

    # atcommand.cpp @ mapflag GM指令的赋值屏蔽处理
    # 按照目前 rAthena 的代码理解, 只有普通的开关型地图标记, 才能被 @mapflag 赋值
    # 其他的地图标记都会禁止使用 @mapflag
    inject.insert(InjectPoint.ATCOMMAND_MAPFLAG_BLOCK, [
        '#ifdef %s' % define,
        '\t\t\tdisabled_mf.insert(disabled_mf.begin(), %s);' % constant,
        '#endif // %s' % define,
        ''
    ])

    # map.cpp @ map_getmapflag_sub 的读取标记处理
    inject.insert(InjectPoint.MAP_GETMAPFLAG_SUB, [
        '#ifdef %s' % define,
        '\t\tcase %s:' % constant,
        '\t\t\treturn map_getmapflag_param(m, mapflag, args, 0);',
        '#endif // %s' % define,
        ''
    ])

    # map.cpp @ map_setmapflag_sub 的设置标记处理
    if default_disable:
        inject.insert(InjectPoint.MAP_SETMAPFLAG_SUB, [
            '#ifdef %s' % define,
            '\t\tcase %s:' % constant,
            '\t\t\tif (!status)',
            '\t\t\t\tmap_setmapflag_param(m, mapflag, %s);' % default_val,
            '\t\t\telse {',
            '\t\t\t\tnullpo_retr(false, args);',
            '\t\t\t\tif (args) {',
            '\t\t\t\t\tmap_setmapflag_param(m, mapflag, args->flag_val);',
            '\t\t\t\t\tstatus = !(args->flag_val == %d);' % default_val,
            '\t\t\t\t}',
            '\t\t\t}',
            '\t\t\tmapdata->flag[mapflag] = status;',
            '\t\t\tbreak;',
            '#endif // %s' % define
        ])
    else:
        inject.insert(InjectPoint.MAP_SETMAPFLAG_SUB, [
            '#ifdef %s' % define,
            '\t\tcase %s:' % constant,
            '\t\t\tif (!status)',
            '\t\t\t\tmap_setmapflag_param(m, mapflag, %s);' % default_val,
            '\t\t\telse {',
            '\t\t\t\tnullpo_retr(false, args);',
            '\t\t\t\tif (args)',
            '\t\t\t\t\tmap_setmapflag_param(m, mapflag, args->flag_val);',
            '\t\t\t}',
            '\t\t\tmapdata->flag[mapflag] = status;',
            '\t\t\tbreak;',
            '#endif // %s' % define
        ])

    # npc.cpp @ npc_parse_mapflag 可选的标记处理
    if default_disable:
        inject.insert(InjectPoint.NPC_PARSE_MAPFLAG, [
            '#ifdef %s' % define,
            '\t\tcase %s: {' % constant,
            '\t\t\t// 若脚本中 mapflag 指定的数值无效或为默认值: %d, 则立刻关闭这个地图标记' % default_val,
            '\t\t\tunion u_mapflag_args args = {};',
            '',
            '\t\t\tif (sscanf(w4, "%11d", &args.flag_val) < 1 || args.flag_val == {default_val} || !state)'.format(default_val = default_val),
            '\t\t\t\tmap_setmapflag(m, mapflag, false);',
            '\t\t\telse',
            '\t\t\t\tmap_setmapflag_sub(m, mapflag, true, &args);',
            '\t\t\tbreak;',
            '\t\t}',
            '#endif // %s' % define,
            ''
        ])
    else:
        inject.insert(InjectPoint.NPC_PARSE_MAPFLAG, [
            '#ifdef %s' % define,
            '\t\tcase %s: {' % constant,
            '\t\t\t// 若脚本中 mapflag 指定的第一个数值参数无效,',
            '\t\t\t// 那么将此参数的值设为 %d, 但不会阻断此地图标记的开启或关闭操作' % default_val,
            '\t\t\tunion u_mapflag_args args = {};',
            '',
            '\t\t\tif (sscanf(w4, "%11d", &args.flag_val) < 1)',
            '\t\t\t\targs.flag_val = %d;' % default_val,
            '',
            '\t\t\tmap_setmapflag_sub(m, mapflag, state, &args);',
            '\t\t\tbreak;',
            '\t\t}',
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
            'name' : '携带单个数值参数的地图标记',
            'desc' : '1 - 携带单个数值参数的地图标记, 可以记录数值变量 (例如 bexp 标记)'
        }
    ]

    # flagtype = 0  # 0 为普通开关 | 1 为数值开关
    flagtype = Inputer().requireSelect({
        'prompt' : '想创建的地图标记类型',
        'option_name' : '地图标记类型',
        'data' : flaglist
    })

    # --------

    default_val = 0
    if flagtype == 1:
        default_val = Inputer().requireInt({
            'tips' : '请输入"第一个数值参数"的默认值 (不填则默认为 0)',
			'allow_empty' : True
        })

    # --------

    default_disable = False
    if flagtype == 1:
        default_disable = Inputer().requireBool({
            'tips' : '当"第一个数值参数"的值为 %d 时, 是否表示移除此地图标记?' % default_val,
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
    Message.ShowInfo('第一个数值参数的默认值 : %d' % default_val)
    Message.ShowInfo('第一个数值参数的值为 %d 时, 是否禁用此标记 : %s' % (default_val, default_disable))
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
        'default_val' : default_val,
        'default_disable' : default_disable
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
                'id' : InjectPoint.ATCOMMAND_MAPFLAG_BLOCK,
                'desc' : 'atcommand.cpp @ mapflag GM指令的赋值屏蔽处理'
            },
            {
                'id' : InjectPoint.MAP_GETMAPFLAG_SUB,
                'desc' : 'map.cpp @ map_getmapflag_sub 的读取标记处理'
            },
            {
                'id' : InjectPoint.MAP_SETMAPFLAG_SUB,
                'desc' : 'map.cpp @ map_setmapflag_sub 的设置标记处理'
            },
            {
                'id' : InjectPoint.NPC_PARSE_MAPFLAG,
                'desc' : 'npc.cpp @ npc_parse_mapflag 可选的标记处理'
            },
            {
                'id' : InjectPoint.ATCOMMAND_MAPINFO,
                'desc' : 'atcommand.cpp @ ACMD_FUNC(mapinfo) 可选的地图信息输出'
            },
            {
                'id' : InjectPoint.SCRIPT_SETMAPFLAG,
                'desc' : 'script.cpp @ setmapflag 可选的脚本设置标记参数处理代码'
            },
            {
                'id' : InjectPoint.SCRIPT_GETMAPFLAG,
                'desc' : 'script.cpp @ getmapflag 可选的脚本读取标记参数处理代码'
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
