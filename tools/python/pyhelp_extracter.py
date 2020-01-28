'''
//===== Pandas Python Script ============================== 
//= 终端翻译对照表提取脚本
//===== By: ================================================== 
//= Sola丶小克
//===== Current Version: ===================================== 
//= 1.0
//===== Description: ========================================= 
//= 此脚本用于快速提取终端汉化的对照表
//===== Additional Comments: ================================= 
//= 1.0 首个版本. [Sola丶小克]
//============================================================
'''

import csv
import os
import re
import yaml
from io import StringIO

from libs import Common

# 切换工作目录为脚本所在目录
os.chdir(os.path.split(os.path.realpath(__file__))[0])

# 工程文件的主目录相对此脚本文件的位置
project_slndir = '../../'

# 需要处理的后缀列表, 除此之外的其他后缀不处理
process_exts = ['.cpp', '.hpp']

# 第一阶段配置, 用于提取字符串的正则表达式
step1_pattern = re.compile(r'^\s*(\/\/|)\s*(%s)\(\s*(.*)\);' % '|'.join([
        'SqlStmt_ShowDebug',
        'Sql_ShowDebug',
        '_vShowMessage',

        'ShowConfigWarning',

        'ShowError',
        'ShowDebug',
        'ShowFatalError',
        'ShowWarning',
        'ShowNotice',
        'ShowInfo',
        'ShowSQL',
        'ShowStatus',
        'ShowMessage',
        
        'strcat'
    ])
)

# 第二阶段配置, CL_WHITE 等常量转换处理相关的正则表达式
step2_rules = [
    # 优先处理 " CL_WHITE " 
    # 这种左侧和右侧都有双引号的情况
    {
        'pattern' : re.compile(r'(\"\s*(CL_[A-Z_]+|PRI[A-Za-z0-9]+|PRtf|EXPAND_AND_QUOTE\((.*)\))\s*\")'),
        'subrepl' : r'[{\2}]'
    },
    # 然后处理 CL_WHITE "
    # 这种左侧没有双引号的情况
    {
        'pattern' : re.compile(r'(\s*(CL_[A-Z_]+|PRI[A-Za-z0-9]+|PRtf|EXPAND_AND_QUOTE\((.*)\))\s*\")'),
        'subrepl' : r'"[{\2}]'
    },
    # 最后处理 " CL_RESET
    # 这种右侧没有双引号的情况
    {
        'pattern' : re.compile(r'(\"\s*(CL_[A-Z_]+|PRI[A-Za-z0-9]+|PRtf|EXPAND_AND_QUOTE\((.*)\))\s*)'),
        'subrepl' : r'[{\2}]"'
    }
]

class quoted(str):
    pass

def quoted_presenter(dumper, data):
    return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='"')
yaml.add_representer(quoted, quoted_presenter)

def step1_extract(filepath):
    '''
    第一阶段
    用于从源代码中提取可能支持翻译处理的字符串
    '''
    match_content = []
    encoding = Common.get_encoding(filepath)
    with open(filepath, 'r', encoding=encoding) as f:
        for line in f.readlines():
            match_result = step1_pattern.findall(line)
            if match_result and match_result[0][0] != r'\\':
                match_content.append({
                    'file' : filepath,
                    'func' : match_result[0][1],
                    'text' : match_result[0][2]
                })
        f.close()
    return match_content

def step2_repl_constants(content):
    '''
    第二阶段
    对 CL_WHITE 之类的常量进行替换处理, 变成 [{CL_WHITE}] 样式
    '''
    for item in content:
        for rule in step2_rules:
            item['text'] = rule['pattern'].sub(rule['subrepl'], item['text'])
    return content

def step3_except_nonstring(content):
    '''
    第三阶段
    去掉提取到内容完全不包含字符串的内容
    例如: ShowWarning(buff);
    '''
    for item in content[:]:
        # 没有任何一个双引号且不以 [{ 开头的字符串则移除
        if item['text'].count('"') == 0 and not str(item['text']).startswith('[{'):
            content.remove(item)
        # 拥有一个双引号且以 [{ 开头的字符串则在左侧补充一个双引号
        if item['text'].count('"') == 1 and str(item['text']).startswith('[{'):
            item['text'] = '"' + item['text']
    return content

def step4_field_offset(content):
    '''
    第四阶段
    部分特殊的函数需要取第二列的内容 (而不是第一列)
    例如: ShowConfigWarning 的第一个参数并不是我们要的提示文本
    '''
    functions = ['ShowConfigWarning', 'strcat']
    for item in content:
        if item['func'] in functions:
            text = str(item['text']).replace(r'\"', r'\""')
            for row in csv.reader(StringIO(text), skipinitialspace=True):
                item['text'] = '"%s"' % str(row[1])
                break
    return content

def step5_drop_params(content):
    '''
    第五阶段
    抛弃参数部分, 只保留第一列的文本内容
    '''
    for item in content:
        text = str(item['text']).replace(r'\"', r'\""')
        for row in csv.reader(StringIO(text), skipinitialspace=True):
            item['text'] = str(row[0])
            break
    return content

def step6_unescape(content):
    '''
    第六阶段
    对转义字符进行一些调整, 避免输出时候错位
    '''
    for item in content:
        item['text'] = item['text'].encode('latin1').decode('unicode-escape')
    return content

def extract(filename):
    content = step1_extract(filename)
    content = step2_repl_constants(content)
    content = step3_except_nonstring(content)
    content = step4_field_offset(content)
    content = step5_drop_params(content)
    content = step6_unescape(content)

    # for i in content:
    #     print(i['text'])

    return content

def formatting(content):
    formated = []
    for x in content:
        formated.append({
            'Original': quoted(x['text']),
            'Translation': quoted('')
        })
    return {'Body' : formated}

def save(content, filename):
    data = formatting(content)
    with open(filename, 'w+') as f:
        yaml.dump(data, f, allow_unicode=True, default_flow_style=False, width=2048)

def main():
    content = []
    for dirpath, _dirnames, filenames in os.walk(project_slndir + 'src'):
        for filename in filenames:
            _base_name, extension_name = os.path.splitext(filename.lower())
            if extension_name.lower() in process_exts:
                extract_data = extract(os.path.normpath('%s/%s' % (dirpath, filename)))
                content.extend(extract_data)
    save(content, 'translation.yml')

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt as _err:
        pass
