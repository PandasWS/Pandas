'''
//===== Pandas Python Script ============================== 
//= 终端翻译对照表提取助手
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
import glob
import os
import re
from io import StringIO

import yaml
from opencc import OpenCC

from libs import Common, Inputer, Message

# 切换工作目录为脚本所在目录
os.chdir(os.path.split(os.path.realpath(__file__))[0])

# 工程文件的主目录相对此脚本文件的位置
project_slndir = '../../'

# 需要处理的后缀列表, 除此之外的其他后缀不处理
process_exts = ['.cpp', '.hpp']

# 第一阶段配置, 用于提取字符串的正则表达式
step1_pattern = re.compile(r'\s*(\/\/|)\s*(%s)\s*\(\s*(.*)\);' % '|'.join([
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
    # 1. 优先处理 " CL_WHITE " 
    # 这种左侧和右侧都有双引号的情况
    {
        'pattern' : re.compile(r'(\"\s*(CL_[A-Z_]+|PRI[A-Za-z0-9]+|PRtf|EXPAND_AND_QUOTE\((.*)\))\s*\")'),
        'subrepl' : r'[{\2}]'
    },
    # 2. 然后处理 CL_WHITE "
    # 这种左侧没有双引号的情况
    {
        'pattern' : re.compile(r'(\s*(CL_[A-Z_]+|PRI[A-Za-z0-9]+|PRtf|EXPAND_AND_QUOTE\((.*)\))\s*\")'),
        'subrepl' : r'"[{\2}]'
    },
    # 3. 在进行 2 处理后需要重新在执行一次 1 处理
    {
        'pattern' : re.compile(r'(\"\s*(CL_[A-Z_]+|PRI[A-Za-z0-9]+|PRtf|EXPAND_AND_QUOTE\((.*)\))\s*\")'),
        'subrepl' : r'[{\2}]'
    },
    # 4. 最后处理 " CL_RESET
    # 这种右侧没有双引号的情况
    {
        'pattern' : re.compile(r'(\"\s*(CL_[A-Z_]+|PRI[A-Za-z0-9]+|PRtf|EXPAND_AND_QUOTE\((.*)\))\s*)'),
        'subrepl' : r'[{\2}]"'
    },
    # 5. 在进行 4 处理后需要重新再执行一次 1 处理
    {
        'pattern' : re.compile(r'(\"\s*(CL_[A-Z_]+|PRI[A-Za-z0-9]+|PRtf|EXPAND_AND_QUOTE\((.*)\))\s*\")'),
        'subrepl' : r'[{\2}]'
    }
]

class quoted(str):
    pass

def quoted_presenter(dumper, data):
    return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='"')

yaml.add_representer(quoted, quoted_presenter)

class TranslationExtracter:
    def __init__(self):
        self.header_version = 1
        self.header_type = 'CONSOLE_TRANSLATE_DB'
        self.body = []
        self.opencc = OpenCC('s2twp')
    
    def __step1_extract_single_file(self, filepath):
        '''
        从指定的源代码文件中, 提取可能支持翻译处理的字符串
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
                        'text' : str(match_result[0][2]).strip()
                    })
            f.close()
        return match_content

    def __step2_replace_constants(self, step1_content):
        '''
        对 CL_WHITE 之类的常量进行替换处理, 变成 [{CL_WHITE}] 样式
        '''
        for item in step1_content:
            for rule in step2_rules:
                item['text'] = rule['pattern'].sub(rule['subrepl'], item['text'])
        return step1_content

    def __step3_except_nonstring(self, step2_content):
        '''
        去掉提取到内容完全不包含字符串的内容, 例如: ShowWarning(buff);
        '''
        for item in step2_content[:]:
            # 没有任何一个双引号且不以 [{ 开头的字符串则移除
            if item['text'].count('"') == 0 and not str(item['text']).startswith('[{'):
                step2_content.remove(item)

            # 拥有一个双引号且以 [{ 开头的字符串则在左侧补充一个双引号
            if item['text'].count('"') == 1 and str(item['text']).startswith('[{'):
                item['text'] = '"' + item['text']
        return step2_content

    def __step4_field_offset(self, step3_content):
        '''
        部分特殊的函数需要取第二列的内容 (而不是第一列)
        例如: ShowConfigWarning 的第一个参数并不是我们要的提示文本
        '''
        functions = ['ShowConfigWarning', 'strcat']
        for item in step3_content:
            if item['func'] in functions:
                text = str(item['text']).replace(r'\"', r'\""')
                for row in csv.reader(StringIO(text), skipinitialspace=True):
                    item['text'] = '"%s"' % str(row[1])
                    break
        return step3_content

    def __step5_drop_params(self, step4_content):
        '''
        抛弃参数部分, 只保留第一列的文本内容
        '''
        for item in step4_content:
            text = str(item['text']).replace(r'\"', r'\""')
            for row in csv.reader(StringIO(text), skipinitialspace=True):
                item['text'] = str(row[0])
                break
        return step4_content

    def __step6_unescape(self, step5_content):
        '''
        对转义字符进行一些调整, 避免输出时候错位
        '''
        for item in step5_content:
            item['text'] = item['text'].encode('latin1').decode('unicode-escape')
        return step5_content
    
    def __make_distinct(self, step6_content):
        '''
        对提取到的内容进行消重处理
        '''
        distincted_keys = list(set([x['text'] for x in step6_content]))
        distincted_keys.sort()
        return [{'text' : k} for k in distincted_keys]

    def __make_body_to_dict(self, body, filter_empty=False):
        '''
        将 body 类型的数据转换成字典类型
        '''
        content = {}
        for x in body:
            if filter_empty and not x['Translation']:
                continue
            content[x['Original']] = x['Translation']
        return content

    def __make_dict_to_body(self, data):
        '''
        将字典类型的数据转换成 body 类型
        '''
        body = []
        for x in data:
            body.append({
                'Original': x,
                'Translation': data[x]
            })
        return body
    
    def __restore_backslash(self, textcontent):
        '''
        针对 BIG5 的处理, 还原文本文件中的反斜杠转义
        '''
        text_processed = ''
        bSkipBackslash = False
        for i, element in enumerate(textcontent):
            if bSkipBackslash and element == '\\':
                bSkipBackslash = False
                continue
            cb = element.encode('big5')
            text_processed = text_processed + element
            if len(cb) == 2 and cb[1] == 0x5C:
                bSkipBackslash = True
        return text_processed
    
    def __convert_backslash_step1(self, textcontent):
        '''
        针对 BIG5 的处理, 在双字节低位等于 0x5C 的字符后面, 插入 [[[\\]]] 标记
        '''
        text_processed = ''
        for i, element in enumerate(textcontent):
            cb = element.encode('big5')
            text_processed = text_processed + element
            if len(cb) == 2 and cb[1] == 0x5C:
                text_processed = text_processed + '[[[\\]]]'
        return text_processed

    def __convert_backslash_step2(self, filepath):
        '''
        针对 BIG5 的处理, 将指定文件中的 [[[\\]]] 替换成反斜杠
        '''
        content = ""
        with open(filepath, 'r', encoding='UTF-8-SIG') as f:
            content = f.read()
            f.close()
        
        pattern = re.compile(r'\[\[\[\\\\\]\]\]')
        content = pattern.sub(r'\\', content)
        
        with open(filepath, 'w', encoding='UTF-8-SIG') as f:
            f.write(content)
            f.close()

    def build(self, src_dir):
        '''
        根据指定的源代码目录, 构建新的数据保存到 self.body 列表
        '''
        Message.ShowStatus('正在扫描源代码目录, 提取可被翻译的字符串...')
        extract_content = []
        for dirpath, _dirnames, filenames in os.walk(src_dir):
            for filename in filenames:
                _base_name, extension_name = os.path.splitext(filename.lower())
                if extension_name.lower() in process_exts:
                    filepath = os.path.normpath('%s/%s' % (dirpath, filename))
                    
                    content = self.__step1_extract_single_file(filepath)
                    content = self.__step2_replace_constants(content)
                    content = self.__step3_except_nonstring(content)
                    content = self.__step4_field_offset(content)
                    content = self.__step5_drop_params(content)
                    content = self.__step6_unescape(content)
                    
                    extract_content.extend(content)
        
        # 对提取到的内容进行消重处理
        extract_content = self.__make_distinct(extract_content)
        
        self.body = []
        for x in extract_content:
            self.body.append({
                'Original': x['text'],
                'Translation': ''
            })
        Message.ShowInfo('扫描完毕, 共有 %d 条可翻译的字符串' % len(self.body))
    
    def load(self, filename):
        '''
        '''
        Message.ShowInfo('正在加载: %s' % filename)
        
        with open(filename, encoding='UTF-8-SIG') as f:
            content = yaml.load(f, Loader=yaml.FullLoader)
            
            header = content['Header']
            self.header_version = header['Version']
            self.header_type = header['Type']
            self.body = content['Body']
        
        Message.ShowInfo('已读取 %d 条记录, 数据版本号为: %d' % (len(self.body), self.header_version))

    def dump(self, filename):
        '''
        将当前 self.body 列表中的内容转储到本地指定文件
        '''
        try:
            Message.ShowInfo('正在保存翻译对照表...')
            _body = []
            for x in self.body:
                _body.append({
                    'Original': quoted(x['Original']),
                    'Translation': quoted(x['Translation'])
                })
            
            restruct = {
                'Header': {
                    'Type': self.header_type,
                    'Version': self.header_version
                },
                'Body' : _body
            }

            with open(filename, 'w+', encoding='UTF-8-SIG') as f:
                yaml.dump(
                    restruct, f, allow_unicode=True,
                    default_flow_style=False, width=2048, sort_keys=False
                )
                f.close()

            self.__convert_backslash_step2(filename)

            Message.ShowInfo('保存到: %s' % os.path.relpath(os.path.abspath(filename), project_slndir))
            return os.path.abspath(filename)
        except Exception as _err:
            print(_err)
            Message.ShowError('保存翻译对照表期间发生错误, 请重试...')
            return None

    def updatefrom(self, from_yml, increase_version=True):
        '''
        读取现有的翻译结果, 刷入 self.body 列表中
        并提升数据的对应版本号
        '''
        content = None
        with open(from_yml, encoding='UTF-8-SIG') as f:
            filecontent = f.read()
            try:
                content = yaml.load(filecontent, Loader=yaml.FullLoader)
            except yaml.scanner.ScannerError as _err:
                content = yaml.load(self.__restore_backslash(filecontent), Loader=yaml.FullLoader)
        
        if content is None:
            return
        
        header = content['Header']
        body = content['Body']
        
        self.header_version = header['Version']
        self.header_type = header['Type']

        from_dict = self.__make_body_to_dict(body, True)
        curr_dict = self.__make_body_to_dict(self.body, False)
        curr_dict.update(from_dict)
        self.body = self.__make_dict_to_body(curr_dict)
        
        if increase_version:
            self.header_version = self.header_version + 1
    
    def updateall(self, increase_version=True):
        '''
        更新全部翻译对照表文件, 并保留现有的翻译结果
        '''
        yamlfiles = glob.glob('../../conf/msg_conf/translation_*.yml')
        
        Message.ShowStatus('即将更新全部翻译对照表, 并保留现有的翻译结果...')
        if increase_version:
            Message.ShowInfo('对照表更新完成后会提升数据版本号.')
        else:
            Message.ShowWarning('本次对照表更新操作不会提升数据版本号.')
        for relpath in yamlfiles:
            fullpath = os.path.abspath(relpath)
            Message.ShowInfo('正在升级: %s' % os.path.relpath(fullpath, project_slndir))
            _backup_body = self.body[:]
            self.updatefrom(fullpath, increase_version)
            if '_tw.yml' in relpath:
                for x in self.body:
                    x['Translation'] = self.__convert_backslash_step1(x['Translation'])
            self.dump(fullpath)
            self.body = _backup_body
        Message.ShowStatus('感谢您的使用, 全部对照表翻译完毕.')

    def toTraditional(self):
        '''
        将当前 self.body 中的译文结果转换到目标结果
        '''
        Message.ShowInfo('正在将译文转换成繁体中文...')
        for x in self.body:
            x['Translation'] = self.opencc.convert(x['Translation'])
            x['Translation'] = self.__convert_backslash_step1(x['Translation'])
        Message.ShowInfo('译文已经顺利转换成繁体中文')

def main():
    Common.welcome('终端翻译对照表提取助手')
    
    options = [
        {
            'name' : '建立新的翻译对照表文件',
            'desc' : '0 - 建立新的翻译对照表文件'
        },
        {
            'name' : '更新现有的翻译对照表文件',
            'desc' : '1 - 更新现有的翻译对照表文件'
        },
        {
            'name' : '基于简体中文汉化结果更新繁体中文数据',
            'desc' : '2 - 基于简体中文汉化结果更新繁体中文数据'
        }
    ]

    userchoose = Inputer().requireSelect({
        'name' : '想要执行的操作或任务',
        'data' : options
    })

    if userchoose == 1:
        updatever = Inputer().requireBool({
            'tips' : '完成更新后是否提升数据版本号?',
            'default' : False
        })
    
    extracter = TranslationExtracter()

    if userchoose == 0:
        extracter.build(project_slndir + 'src')
        extracter.dump('translation.yml')
    elif userchoose == 1:
        extracter.build(project_slndir + 'src')
        extracter.updateall(updatever)
    elif userchoose == 2:
        extracter.load(project_slndir + 'conf/msg_conf/translation_cn.yml')
        extracter.toTraditional()
        extracter.dump(project_slndir + 'conf/msg_conf/translation_tw.yml')
    
    Common.exit_with_pause()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt as _err:
        pass
