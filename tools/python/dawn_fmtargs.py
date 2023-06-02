'''
//===== Pandas Python Script =================================
//= 格式化占位符检查辅助脚本
//===== By: ================================================== 
//= Sola丶小克
//===== Current Version: ===================================== 
//= 1.0
//===== Description: ========================================= 
//= 此脚本用于检查 msg_conf 中各翻译后的消息文件里, 每一条消息
//= 所包含的格式化占位符是否与英文原版一致
//===== Additional Comments: ================================= 
//= 1.0 首个版本. [Sola丶小克]
//============================================================
'''

# -*- coding: utf-8 -*-

import environment
environment.initialize()

import os
import re
import operator

from libs import Common, Message

# 切换工作目录为脚本所在目录
os.chdir(os.path.split(os.path.realpath(__file__))[0])

# 工程文件的主目录相对此脚本文件的位置
project_slndir = '../../'

# 用于提取格式化标记的正则表达式
# 参考: https://www.cplusplus.com/reference/cstdio/printf/
format_pattern = r"(%([+\-#0]|)(\d+|\*|)(\.\d+|\.\*|)[diuoxXfFeEgGaAcspn%]?)"

# 作为入口需要进行分析的文件, 同时也是范本文件
# 它们 import 的其他文件, 都需与他们自己包含的格式化标记的个数、顺序完全一致才行
entrance_configure = [
    'conf/msg_conf/login_msg.conf',
    'conf/msg_conf/char_msg.conf',
    'conf/msg_conf/map_msg.conf'
]

def split_msg(line):
    '''
    对每一行的文本信息进行拆分 (跳过注释行)
    '''
    if line.strip().startswith('//'):
        return None
    fields = line.split(':')
    fields = [x.strip() for x in fields]
    if len(fields) < 2:
        return None
    if len(fields) > 2:
        k = fields[0]
        del fields[0]
        v = ':'.join(fields)
        fields = [k, v]
    return fields

def get_import_files(filename):
    '''
    提供一个文件检查他是否导入了其他文件, 返回它导入的其他文件名称
    通常是基于仓库根目录的相对路径, 如: conf/msg_conf/map_msg_chs.conf
    '''
    with open(filename, 'rt', encoding=Common.get_file_encoding(filename)) as f:
        content = f.readlines()

    import_files = []
    for line in content:
        fields = split_msg(line)
        if fields is None:
            continue
        if fields[0].lower() in ['import', 'import_cht', 'import_chs']:
            import_files.append(fields[1])
    return import_files

def get_recursion_import_files(filename):
    '''
    递归获取指定文件中引入的全部其他文件
    '''
    allfiles = []
    
    import_files = get_import_files(filename)

    for x in import_files:
        if x in allfiles:
            continue
        allfiles.append(x)
        sub_files = get_recursion_import_files(os.path.abspath(project_slndir + x))
        allfiles.extend(sub_files)
        allfiles = list(set(allfiles))

    return allfiles

def get_format_specifier(fmt):
    '''
    给出一个字符串, 提取出其中用 % 开头的格式化标记.
    返回一个列表
    '''
    result = re.findall(format_pattern, fmt)
    result = [x[0] for x in result]
    return result

def get_mes_dict(filename):
    '''
    给定一个消息文件, 读取其中的所有信息并返回一个字典
    '''
    with open(filename, 'rt', encoding=Common.get_file_encoding(filename)) as f:
        content = f.readlines()

    mes_dict = {}
    for line in content:
        fields = split_msg(line)
        if fields is None:
            continue
        if not fields[0].isdigit():
            continue
        mes_dict[int(fields[0])] = {
            'mes': fields[1],
            'fmt': get_format_specifier(fields[1])
        }
    return mes_dict

def do_check_file(filename):
    '''
    检查给定的消息文件
    '''
    # 加载主要的消息文件
    Message.ShowStatus('正在加载 {filename} 的消息内容...'.format(
        filename = os.path.relpath(filename, project_slndir)
    ))

    master = get_mes_dict(filename)

    # 递归寻找与他相关的 import 文件
    import_files = get_recursion_import_files(filename)

    Message.ShowStatus('读取完毕, 找到约 {infocnt} 条消息信息, 额外引入了 {importcnt} 个消息文件.'.format(
        infocnt = len(master), importcnt = len(import_files)
    ))

    # 分别加载这些文件的全部消息信息
    b_allfine = True
    for x in import_files:
        compare = get_mes_dict(os.path.abspath(project_slndir + x))

        # 若消息存在于主文件, 那么比对格式化标记
        for mesid in compare:
            if mesid not in master.keys():
                continue

            if not operator.eq(master[mesid]['fmt'], compare[mesid]['fmt']):
                b_allfine = False

                Message.ShowWarning('发现消息编号为: {mesid} 的格式化标记与主文件不一致!'.format(
                    mesid = mesid
                ))

                Message.ShowInfo('  主要 - {filename}: {mes}'.format(
                    filename = os.path.basename(filename),
                    mes = master[mesid]['mes']
                ))

                Message.ShowInfo('  对比 - {filename}: {mes}'.format(
                    filename = os.path.basename(x),
                    mes = compare[mesid]['mes']
                ))

                print('')
    
    if b_allfine:
        Message.ShowStatus('检查完毕, 消息信息的格式化标记与主文件完全匹配!')

    print('')

def main():
    # 显示欢迎信息
    Common.welcome('格式化占位符检查辅助脚本')

    print('')
    
    # 执行检查工作
    for i in entrance_configure:
        do_check_file(os.path.abspath(project_slndir + i))
    
    # 友好退出, 主要是在 Windows 环境里给予暂停
    Common.exit_with_pause()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt as _err:
        pass
