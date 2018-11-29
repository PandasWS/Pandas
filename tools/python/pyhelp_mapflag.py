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
import platform

import chardet


class InjectMarkController:
    def __init__(self, options):
        self.__options__ = options

    def __detectCharset(self, filepath):
        '''
        给定一个文件路径, 获取该文本文件的编码
        '''
        with open(filepath, 'rb') as hfile:
            return chardet.detect(hfile.read())['encoding']

    def __search_mark(self, filename):
        line_num = 0
        charset = self.__detectCharset(filename)
        if self.__detectCharset(filename).upper() != 'UTF-8-SIG':
            print('No UTF-8-SIG: %s' % (filename))
            return
        try:
            textfile = open(filename, encoding=charset)
            for line in textfile:
                line_num = line_num + 1
                if '//' not in line:
                    continue

                for mark in self.__options__['mark_list']:
                    if self.__options__['mark_format'] % mark['index'] in line:
                        mark['filepath'] = filename
                        mark['line'] = line_num
            textfile.close()
        except Exception as err:
            print('Error filename : %s | Message : %s' % (filename, err))

    def detect(self, src_dir):
        for dirpath, _dirnames, filenames in os.walk(src_dir):
            for filename in filenames:
                _base_name, extension_name = os.path.splitext(filename.lower())

                if extension_name not in self.__options__['process_exts']:
                    continue

                fullpath = os.path.normpath('%s/%s' % (dirpath, filename))
                self.__search_mark(fullpath)

        bMarkPassed = True
        for mark in self.__options__['mark_list']:
            if mark['filepath'] is None or mark['line'] is None:
                bMarkPassed = False

        return bMarkPassed

    def print(self):
        for mark in self.__options__['mark_list']:
            print('Insert Point {insertID} in File {fullPath} at Line {lineNumber}'.format(
                insertID = mark['index'],
                fullPath = mark['filepath'],
                lineNumber = mark['line']
            ))

    def insert(self, index, content):
        index = index - 1
        insert_content = '\n'.join(content)
        insert_content = insert_content + '\n'
        mark_list = self.__options__['mark_list']

        charset = self.__detectCharset(mark_list[index]['filepath'])
        rfile = open(mark_list[index]['filepath'], encoding=charset)
        filecontent = rfile.readlines()
        rfile.close()

        filecontent.insert(mark_list[index]['line'] - 1, insert_content)

        wfile = open(mark_list[index]['filepath'], mode = 'w', encoding=charset)
        wfile.writelines(filecontent)
        wfile.close()

        # 此处需要更新其他 mark 的行数
        # 避免本次插入导致他们原来记录的 line 错误

        for mark in mark_list:
            if mark['index'] == index:
                continue

            if mark['filepath'].lower() != mark_list[index]['filepath'].lower():
                continue

            if mark_list[index]['line'] < mark['line']:
                mark['line'] = mark['line'] + len(content)

def insert_for_normal_mapflag(inject, options, special = True):
    define = options['define']
    constant = options['constant']

    # rathena.hpp @ 宏定义
    inject.insert(1, [
        '',
        '\t// 是否启用 %s 地图标记 [维护者昵称]' % constant.lower().replace('mf_'),
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
        '\tint %s,' % var_name_1,
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
    print('地图标记添加助手')
    print('=' * 70)

    print('[信息] 在使用此脚本之前, 建议确保 src 目录的工作区是干净的.')
    print('[信息] 这样添加结果如果不符合预期, 可以轻松的利用 git 进行重置操作.')

def guide(inject):
    print('[选择] 请输入该地图标记的宏定义开关名称 (rAthenaCN_MapFlag_的末尾部分):')
    define = input('rAthenaCN_MapFlag_')
    define = 'rAthenaCN_MapFlag_' + define
    print('[提示] 您输入的是: ' + define)
    print('')

    # --------

    print('[选择] 请输入该地图标记的 MF 常量名称 (自动大写, MF_的末尾部分):')
    constant = input('MF_')
    constant = 'MF_' + constant.upper()
    print('[提示] 您输入的是: %s' % constant)
    print('')

    # --------

    flagtype = 0  # 0 为普通开关 | 1 为数值开关
    flaglist = [
        { 'name' : '普通开关式的地图标记', 'desc' : '0 - 普通的地图标记开关, 只有两个状态(开/关)' },
        { 'name' : '携带单个数值参数的地图标记', 'desc' : '1 - 携带单个数值参数的地图标记, 可以记录数值变量 (例如 bexp 标记)' },
        { 'name' : '复杂赋值标记', 'desc' : '2 - 复杂的赋值标记, 只单纯添加宏定义和 MF 常量, 其他的手动添加' }
    ]

    print('[提示] 请选择地图标记的类型, 可选值为 [0~2], 分别代表:')
    for flag in flaglist:
        print('[提示]     %s' % flag['desc'])
    user_sel = int(input('[选择] 请选择想创建的地图标记类型 [0~2]:'))

    if user_sel < 0 or user_sel > len(flaglist):
        print('[提示] 您输入了无效的地图标记类型, 程序终止')
        exit(-1)
    flagtype = user_sel
    print('[提示] 您选择的是: %s' % flaglist[flagtype]['name'])
    print('')

    # --------

    var_name_1 = None
    if flagtype == 1:
        print('[选择] 请输入用于记录"第一个数值参数"的 map_data 结构成员变量名:')
        var_name_1 = input('')
        print('[提示] 您输入的是: %s' % var_name_1)
        print('')

    # --------

    zero_disable = False
    if flagtype == 1:
        user_sel = input('[选择] 当"第一个数值参数"的值为 0 时, 是否表示移除此地图标记? [Y/N]:')
        if user_sel.lower() in ['y', 'yes']:
            zero_disable = True
        elif user_sel.lower() in ['n', 'no']:
            zero_disable = False
        else:
            print('[提示] 您输入了无效的选项, 程序终止')
            exit(-1)
    print('')

    # --------

    print('-' * 70)
    print('[信息] 请确认建立的信息, 确认后将开始修改代码.')
    print('-' * 70)
    print('[信息] 开关名称 : %s' % define)
    print('[信息] 常量名称 : %s' % constant)
    print('[信息] 标记类型 : %s' % flaglist[flagtype]['name'])
    print('')
    print('[信息] 第一个数值参数名称 : %s' % var_name_1)
    print('[信息] 第一个数值参数的值为 0 时, 是否禁用此标记 : %s' % zero_disable)
    print('-' * 70)

    user_sel = input('[选择] 确认要开始写入操作么? [Y/N]:')
    if user_sel.lower() in ['y', 'yes']:
        pass
    elif user_sel.lower() in ['n', 'no']:
        print('[信息] 终止写入操作, 程序终止')
        exit(-1)
    else:
        print('[信息] 您输入了无效的选项, 程序终止')
        exit(-1)
    print('')

    # --------

    print('[信息] 开始写入操作...')

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

    print('[信息] 写入完成, 请检查代码并补充注释.')

def main():
    os.chdir(os.path.split(os.path.realpath(__file__))[0])

    welecome()

    options = {
        'process_exts' : ['.hpp', '.cpp'],
        'mark_format' : '// PYHELP - MAPFLAG - INSERT POINT - <Section %d>',
        'mark_list' : [
            { 'index' : 1,  'filepath' : None, 'line' : None },
            { 'index' : 2,  'filepath' : None, 'line' : None },
            { 'index' : 3,  'filepath' : None, 'line' : None },
            { 'index' : 4,  'filepath' : None, 'line' : None },
            { 'index' : 5,  'filepath' : None, 'line' : None },
            { 'index' : 6,  'filepath' : None, 'line' : None },
            { 'index' : 7,  'filepath' : None, 'line' : None },
            { 'index' : 8,  'filepath' : None, 'line' : None },
            { 'index' : 9,  'filepath' : None, 'line' : None },
            { 'index' : 10, 'filepath' : None, 'line' : None },
            { 'index' : 11, 'filepath' : None, 'line' : None }
        ]
    }

    imc = InjectMarkController(options)
    if not imc.detect('../../src'):
        print('[状态] 无法成功定位所有需要的代码注入点, 程序终止!')
        exit(-1)
    else:
        print('[状态] 已成功定位所有代码注入点.\n')

    guide(imc)

    if platform.system() == 'Windows':
        os.system('pause')

if __name__ == '__main__':
    main()
