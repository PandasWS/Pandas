'''
//===== Pandas Python Script =================================
//= 游戏文本转译辅助脚本
//===== By: ================================================== 
//= Sola丶小克
//===== Current Version: ===================================== 
//= 1.0
//===== Description: ========================================= 
//= 此脚本用于在发布时自动替换物品、魔物、技能名称为中文版
//===== Additional Comments: ================================= 
//= 1.0 首个版本. [Sola丶小克]
//============================================================
'''

# -*- coding: utf-8 -*-

import environment
environment.initialize()

import glob
import os
import re

from ruamel.yaml import YAML, scalarstring
from libs import Common, Inputer, Message
from opencc import OpenCC

# 切换工作目录为脚本所在目录
os.chdir(os.path.split(os.path.realpath(__file__))[0])

# 工程文件的主目录相对此脚本文件的位置
project_slndir = '../../'

# 配置需要进行自动汉化处理的相关信息
configures = [
    {
        'operate' : 'LineReplaceController',
        'operate_params' : {
            'transdb_name' : 'itemname',
            'pattern' : r'(//|)(\d+)(,.*?,)(.*?)(,.*?)$',
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>\g<5>',
            'id_pos' : 2,
            'replace_pos' : 4,
            'replace_escape' : False,
            'save_encoding' : 'UTF-8-SIG'
        },
        'filepath' : [
            'db/re/item_db.txt',
            'db/pre-re/item_db.txt'
        ]
    },
    {
        'operate' : 'LineReplaceController',
        'operate_params' : {
            'transdb_name' : 'mobname',
            'pattern' : r'(//|)(\d+)(,.*?,)(.*?)(,.*?)$',
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>\g<5>',
            'id_pos' : 2,
            'replace_pos' : 4,
            'replace_escape' : False,
            'save_encoding' : 'UTF-8-SIG'
        },
        'filepath' : [
            'db/re/mob_db.txt',
            'db/pre-re/mob_db.txt'
        ]
    },
    {
        'operate' : 'LineReplaceController',
        'operate_params' : {
            'transdb_name' : 'mobname',
            'pattern' : r'(//|)(.*?,)(\d+)(,.*?)(.*?)(,.*)',
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>\g<5>\g<6>',
            'id_pos' : 3,
            'replace_pos' : 5,
            'replace_escape' : False,
            'save_encoding' : 'UTF-8-SIG'
        },
        'filepath' : [
            'db/re/mob_boss.txt',
            'db/pre-re/mob_boss.txt',
            'db/re/mob_branch.txt',
            'db/pre-re/mob_branch.txt',
            'db/re/mob_poring.txt',
            'db/pre-re/mob_poring.txt',
            'db/mob_classchange.txt',
            'db/mob_mission.txt',
            'db/mob_pouch.txt'
        ]
    },
    {
        'operate' : 'LineReplaceController',
        'operate_params' : {
            'transdb_name' : 'itemname',
            'pattern' : r'(//.*?|.*?)(\d+)(.*?//)(.*)',
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>',
            'id_pos' : 2,
            'replace_pos' : 4,
            'replace_escape' : False,
            'replace_decorate' : 'CommentSpaceStandard',
            'save_encoding' : 'UTF-8-SIG'
        },
        'filepath' : [
            'db/re/item_flag.txt',
            'db/pre-re/item_flag.txt',
            'db/re/item_delay.txt',
            'db/pre-re/item_delay.txt',
            'db/re/item_buyingstore.txt',
            'db/pre-re/item_buyingstore.txt',
            'db/re/item_noequip.txt',
            'db/pre-re/item_noequip.txt',
            'db/re/item_stack.txt',
            'db/pre-re/item_stack.txt',
            'db/re/item_trade.txt',
            'db/pre-re/item_trade.txt',
            'db/item_avail.txt',
            'db/item_nouse.txt'
        ]
    },
    {
        'operate' : 'LineReplaceController',
        'operate_params' : {
            'transdb_name' : 'itemname',
            'pattern' : r'(//.*?,|.*?,)(\d+)(.*?//)(.*)',
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>',
            'id_pos' : 2,
            'replace_pos' : 4,
            'replace_escape' : False,
            'replace_decorate' : 'CommentSpaceStandard',
            'save_encoding' : 'UTF-8-SIG'
        },
        'filepath' : [
            'db/re/item_bluebox.txt',
            'db/pre-re/item_bluebox.txt',
            'db/re/item_cardalbum.txt',
            'db/pre-re/item_cardalbum.txt',
            'db/re/item_violetbox.txt',
            'db/pre-re/item_violetbox.txt',
            'db/re/item_giftbox.txt',
            'db/pre-re/item_giftbox.txt',
            'db/re/item_misc.txt',
            'db/pre-re/item_misc.txt',
            'db/re/item_package.txt',
            'db/item_findingore.txt'
        ]
    },
    {
        'operate' : 'LineReplaceController',
        'operate_params' : {
            'transdb_name' : 'itemname',
            'pattern' : r"(.*?\()(\d+)(,.*?,')(.*?)(',.*)",
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>\g<5>',
            'id_pos' : 2,
            'replace_pos' : 4,
            'replace_escape' : True,
            'save_encoding' : 'UTF-8'
        },
        'globpath' : [
            'sql-files/**/*item_db*.sql'
        ],
        'globexclude' : [
            'sql-files/item_db.sql',
            'sql-files/item_db_re.sql',
            'sql-files/compatibility',
            'sql-files/tools'
        ]
    },
    {
        'operate' : 'LineReplaceController',
        'operate_params' : {
            'transdb_name' : 'mobname',
            'pattern' : r"(.*?\()(\d+)(,.*?)(,.*?,')(.*?)(',.*)",
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>\g<5>\g<6>',
            'id_pos' : 2,
            'replace_pos' : 5,
            'replace_escape' : True,
            'save_encoding' : 'UTF-8'
        },
        'globpath' : [
            'sql-files/**/*mob_db*.sql'
        ]
    },
    {
        'operate' : 'LineReplaceController',
        'operate_params' : {
            'transdb_name' : 'skillname',
            'pattern' : r'(//.*?|)(\d+)(.*?//)(.*)',
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>',
            'id_pos' : 2,
            'replace_pos' : 4,
            'replace_escape' : False,
            'replace_decorate' : 'CommentSpaceStandard',
            'save_encoding' : 'UTF-8-SIG'
        },
        'filepath' : [
            'db/re/skill_nocast_db.txt',
            'db/pre-re/skill_nocast_db.txt'
        ]
    },
    {
        'operate' : 'LineReplaceController',
        'operate_params' : {
            'transdb_name' : 'skillname',
            'pattern' : r'(//.*?|)(\d+,)(\d+)(.*?//)(.*)',
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>\g<5>',
            'id_pos' : 3,
            'replace_pos' : 5,
            'replace_escape' : False,
            'replace_decorate' : 'SkillTreeDescription',
            'save_encoding' : 'UTF-8-SIG'
        },
        'filepath' : [
            'db/re/skill_tree.txt',
            'db/pre-re/skill_tree.txt'
        ]
    },
    {
        'operate' : 'LineReplaceController',
        'operate_params' : {
            'transdb_name' : 'skillname',
            'pattern' : r'(//.*?|)(.*?,)(.*?)(,.*?,)(\d+)(,.*)',
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>\g<5>\g<6>',
            'id_pos' : 5,
            'replace_pos' : 3,
            'replace_escape' : False,
            'replace_decorate' : 'MobSkillForSkillName',
            'save_encoding' : 'UTF-8-SIG'
        },
        'filepath' : [
            'db/re/mob_skill_db.txt',
            'db/pre-re/mob_skill_db.txt'
        ]
    },
    {
        'operate' : 'LineReplaceController',
        'operate_params' : {
            'transdb_name' : 'mobname',
            'pattern' : r'(//.*?|)(.*?)(,.*?)(,.*?,)(\d+)(,.*)',
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>\g<5>\g<6>',
            'id_pos' : 2,
            'replace_pos' : 3,
            'replace_escape' : False,
            'replace_decorate' : 'MobSkillForMobName',
            'save_encoding' : 'UTF-8-SIG'
        },
        'filepath' : [
            'db/re/mob_skill_db.txt',
            'db/pre-re/mob_skill_db.txt'
        ]
    },
    {
        'operate' : 'LineReplaceController',
        'operate_params' : {
            'transdb_name' : 'skillname',
            'pattern' : r"(.*?\()(\d+)(,.*?)(,'.*?',)(\d+)(,.*)",
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>\g<5>\g<6>',
            'id_pos' : 5,
            'replace_pos' : 3,
            'replace_escape' : True,
            'replace_decorate' : 'MobSkillForSkillNameSQL',
            'save_encoding' : 'UTF-8-SIG'
        },
        'globpath' : [
            'sql-files/**/*mob_skill_db*.sql'
        ]
    },
    {
        'operate' : 'LineReplaceController',
        'operate_params' : {
            'transdb_name' : 'mobname',
            'pattern' : r"(.*?\()(\d+)(,.*?)(,'.*?',)(\d+)(,.*)",
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>\g<5>\g<6>',
            'id_pos' : 2,
            'replace_pos' : 3,
            'replace_escape' : True,
            'replace_decorate' : 'MobSkillForMobNameSQL',
            'save_encoding' : 'UTF-8-SIG'
        },
        'globpath' : [
            'sql-files/**/*mob_skill_db*.sql'
        ]
    },
    {
        'operate' : 'YamlReplaceController',
        'operate_params' : {
            'transdb_name' : 'itemname',
            'id_field' : 'Id',
            'target_field' : 'Name',
            'replace_escape' : False,
            'replace_decorate' : 'YamlDoubleQuotedHandling',
            'save_encoding' : 'UTF-8-SIG'
        },
        'globpath' : [
            'db/pre-re/item_db_*.yml',
            'db/re/item_db_*.yml'
        ]
    },
    {
        'operate' : 'YamlReplaceController',
        'operate_params' : {
            'transdb_name' : 'skillname',
            'id_field' : 'Id',
            'target_field' : 'Description',
            'replace_escape' : False,
            'replace_decorate' : 'YamlDoubleQuotedHandling',
            'save_encoding' : 'UTF-8-SIG'
        },
        'globpath' : [
            'db/pre-re/skill_db.yml',
            'db/re/skill_db.yml'
        ]
    },
    {
        'operate' : 'YamlReplaceController',
        'operate_params' : {
            'transdb_name' : 'questname',
            'id_field' : 'Id',
            'target_field' : 'Title',
            'replace_escape' : False,
            'replace_decorate' : 'YamlDoubleQuotedHandling',
            'save_encoding' : 'UTF-8-SIG'
        },
        'globpath' : [
            'db/pre-re/quest_db.yml',
            'db/re/quest_db.yml'
        ]
    },
    {
        'operate' : 'FulltextReplaceController',
        'operate_params' : {
            'transdb_name' : 'groupname',
            'pattern' : r'("|\[)(.*?)("|\])',
            'replace_sub' : r'\g<1>{trans}\g<3>',
            'id_pos' : 2,
            'replace_escape' : False,
            'regex_flags' : re.DOTALL | re.MULTILINE,
            'save_encoding' : 'UTF-8-SIG',
            'big5_escape' : False
        },
        'filepath' : [
            'conf/groups.conf'
        ]
    },
    {
        'operate' : 'FulltextReplaceController',
        'operate_params' : {
            'transdb_name' : 'channelname',
            'pattern' : r'(name:\s+|alias:\s+)"(.*?)"',
            'replace_sub' : r'\g<1>"{trans}"',
            'id_pos' : 2,
            'replace_escape' : False,
            'regex_flags' : re.DOTALL | re.MULTILINE,
            'save_encoding' : 'UTF-8-SIG',
            'big5_escape' : False
        },
        'filepath' : [
            'conf/channels.conf'
        ]
    },
    {
        'operate' : 'FulltextReplaceController',
        'operate_params' : {
            'transdb_name' : 'storagename',
            'pattern' : r'Name:(\s+)"(.*?)"',
            'replace_sub' : r'Name:\g<1>"{trans}"',
            'id_pos' : 2,
            'replace_escape' : False,
            'regex_flags' : re.DOTALL | re.MULTILINE,
            'save_encoding' : 'UTF-8-SIG',
            'big5_escape' : False
        },
        'filepath' : [
            'conf/inter_server.yml'
        ]
    },
    {
        'operate' : 'YamlReplaceController',
        'operate_params' : {
            'transdb_name' : 'mobname',
            'id_field' : 'Id',
            'target_field' : 'JapaneseName',
            'target_pos' : 3,
            'replace_escape' : False,
            'replace_decorate' : 'YamlDoubleQuotedHandling',
            'save_encoding' : 'UTF-8-SIG'
        },
        'globpath' : [
            'db/pre-re/mob_db.yml',
            'db/re/mob_db.yml'
        ]
    },
]

# 配置需要将其转换成繁体中文的路径
need_s2tw_convert = [
    r'conf\*',
    r'conf\battle\*',
    r'doc\*',
    r'db\*.yml',
    r'db\*.txt',
    r'db\import-tmpl\*.yml',
    r'db\import-tmpl\*.txt',
    r'db\pandas\**',
    r'db\re\*.yml',
    r'db\re\*.txt',
    r'db\pre-re\*.yml',
    r'db\pre-re\*.txt',
    r'sql-files\composer\**\**\*',
    r'tools\python\dist\**\*'
]

only_convert_comment = [
    r'.*\\db\\.*(\.yml|\.txt)$'
]


def MobSkillForSkillName(origin, target):
    fields = origin.split('@')
    if len(fields) != 2:
        return target
    return '%s@%s' % (fields[0].strip(), target)

def MobSkillForMobName(origin, target):
    fields = origin.split('@')
    if len(fields) != 2:
        return target
    return ',%s@%s' % (target, fields[1].strip())

def MobSkillForSkillNameSQL(origin, target):
    fields = origin.split('@')
    if len(fields) != 2:
        return target
    return '%s@%s\'' % (fields[0].strip(), target)

def MobSkillForMobNameSQL(origin, target):
    fields = origin.split('@')
    if len(fields) != 2:
        return target
    return ',\'%s@%s' % (target, fields[1].strip())

def SkillTreeDescription(origin, target):
    fields = origin.split('#')
    if len(fields) != 3:
        return target
    return ' %s#%s#' % (fields[0].strip(), target)

def CommentSpaceStandard(origin, target):
    return ' %s' % target.strip()

def YamlDoubleQuotedHandling(origin, target):
    special = ['[', ']']
    isSpecial = False
    for x in special:
        if x in target:
            isSpecial = True
            break
    if isSpecial:
        return scalarstring.DoubleQuotedScalarString(target)
    else:
        return target

def convert_backslash_step1(textcontent):
    '''
    针对 BIG5 的处理, 在双字节低位等于 0x5C 的字符后面, 插入 [[[\\]]] 标记
    '''
    text_processed = ''
    for i, element in enumerate(textcontent):
        cb = element.encode('big5')
        text_processed = text_processed + element
        if len(cb) == 2 and cb[1] == 0x5C:
            text_processed = text_processed + r'[[[\\]]]'
    return text_processed

def convert_backslash_step2(filepath):
    '''
    针对 BIG5 的处理, 将指定文件中的 [[[\\]]] 替换成反斜杠
    '''
    content = ""
    with open(filepath, 'r', encoding='UTF-8') as f:
        content = f.read()
        f.close()
    
    pattern = re.compile(r'\[\[\[\\\\\]\]\]')
    content = pattern.sub(r'\\', content)
    
    with open(filepath, 'w', encoding='UTF-8') as f:
        f.write(content)
        f.close()

def s2twp_convert(filepath):
    '''
    将指定的文件转换成繁体中文
    '''
    if not os.path.exists(filepath):
        return
    
    if not os.path.isfile(filepath):
        return

    cc = OpenCC('s2twp')
    contents = []
    encoding = Common.get_file_encoding(filepath)
    encoding = 'latin1' if encoding is None else encoding
    with open(filepath, 'r', encoding=encoding) as f:
        contents = f.readlines()
    
    file_ext = Common.get_file_ext(filepath)
    file_ext = file_ext.lower()
    only_comment = False

    for pattern in only_convert_comment:
        matches = re.match(pattern, filepath)
        if matches:
            only_comment = True
            break

    trans_contents = []
    for line in contents:
        if not only_comment:
            trans_contents.append(cc.convert(line))
        else:
            if file_ext == '.yml' and line.strip().startswith('#'):
                trans_contents.append(cc.convert(line))
            elif file_ext == '.txt' and line.strip().startswith('//'):
                trans_contents.append(cc.convert(line))
            else:
                trans_contents.append(line)

    with open(filepath, 'w', encoding='UTF-8-SIG') as f:
        for x in trans_contents:
            f.write(x)

class TranslateDatabase():
    def __init__(self, name, lang = 'zh-cn'):
        self.__loaded = False
        self.__translateDict = {}
        self.__lang = lang
        self.__filename = os.path.abspath('./db/%s/%s.txt' % (lang, name))
        self.__load(self.__filename)
    
    def __encoding_check(self, text, encoding, line):
        try:
            for i, element in enumerate(text):
                element.encode(encoding)
        except Exception as _err:
            Message.ShowError('%s 第 %-5d 行中的 "%s" 字符不存在于 "%s" 编码中, 此错误必须被消除' % (
                os.path.relpath(self.__filename), line, element, encoding
            ))

    def __load(self, filename):
        if not Common.is_file_exists(filename):
            raise Exception('翻译对照表不存在: %s' % os.path.relpath(filename))

        # 将文件中的内容读取成一个列表, 放在 contents 中
        contents = []
        with open(filename, 'r', encoding='UTF-8') as f:
            contents = f.readlines()

        # 进行编码校验工作
        if self.__lang == 'zh-tw':
            for i, element in enumerate(contents):
                self.__encoding_check(element, 'big5', i + 1)
        elif self.__lang == 'zh-cn':
            for i, element in enumerate(contents):
                self.__encoding_check(element, 'gbk', i + 1)

        # 遍历处理列表中的全部内容, 让他们每一行都去掉左右的空格
        contents = [x.strip() for x in contents]

        # 移除内容中有反斜杠的行
        contents = [x for x in contents if not x.startswith('//')]

        # 把列表中的内容加工成字典, 若 ID 冲突则以最后读取到的为准
        for line in contents:
            fields = line.split(',')

            if len(fields) != 2:
                continue
            
            if not fields[0]:
                continue

            if not fields[1]:
                continue

            mapid = str(fields[0])
            self.__translateDict[mapid] = fields[1]
        self.__loaded = True
    
    def trans(self, id):
        if not self.__loaded:
            raise Exception('进行文本转译之前, 请先完成初始化...')

        if str(id) in self.__translateDict:
            return self.__translateDict[str(id)]

        return None

class LineReplaceController():
    def __init__(self, **kwargs):
        self.__id_pos = self.__getfromdict(kwargs, 'id_pos')
        self.__replace_pos = self.__getfromdict(kwargs, 'replace_pos')
        self.__pattern = self.__getfromdict(kwargs, 'pattern')
        self.__replace_sub = self.__getfromdict(kwargs, 'replace_sub')
        self.__save_encoding = self.__getfromdict(kwargs, 'save_encoding')
        self.__replace_escape = self.__getfromdict(kwargs, 'replace_escape')
        self.__project_dir = self.__getfromdict(kwargs, 'project_dir')
        self.__lang = self.__getfromdict(kwargs, 'lang')
        self.__silent = self.__getfromdict(kwargs, 'silent')
        self.__transdb_name = self.__getfromdict(kwargs, 'transdb_name')
        self.__replace_decorate = self.__getfromdict(kwargs, 'replace_decorate')
        self.__regex_flags = self.__getfromdict(kwargs, '__regex_flags', 0)
        self.__big5_escape = self.__getfromdict(kwargs, 'big5_escape', False)
        self.__trans = TranslateDatabase(self.__transdb_name, self.__lang)
    
    def __getfromdict(self, dictmap, key, default = None):
        if key not in dictmap:
            return default
        return dictmap[key]

    def __load(self, filename):
        # 将文件中的内容读取成一个列表, 放在 contents 中
        contents = []

        encoding = Common.get_file_encoding(filename)
        encoding = 'latin1' if encoding is None else encoding

        with open(filename, 'r', encoding=encoding) as f:
            contents = f.readlines()
        return contents
    
    def __escape(self, value):
        if value is None:
            return None

        escapelist = ['\'']
        escape_val = ''
        for c in value:
            if c in escapelist:
                escape_val += r'\\' +  c
            else:
                escape_val += c
        return escape_val

    def __process(self, contents):
        for k,line in enumerate(contents):
            matches = re.match(self.__pattern, line)
            if not matches:
                continue
            
            if not matches.groups()[self.__id_pos - 1].isdigit():
                continue

            mapid = int(matches.groups()[self.__id_pos - 1])
            transname = self.__trans.trans(mapid)

            if not transname:
                continue
            
            if self.__replace_escape:
                transname = self.__escape(transname)

            if self.__replace_decorate is not None:
                transname = globals()[self.__replace_decorate](
                    matches.groups()[self.__replace_pos - 1], transname
                )

            repl = self.__replace_sub
            repl = repl.replace('\\g<%d>' % self.__replace_pos, transname)
            contents[k] = re.sub(self.__pattern, repl, line, flags=self.__regex_flags)
        return contents

    def __save(self, contents, filename):
        try:
            if self.__lang == 'zh-tw' and self.__big5_escape:
                contents = [convert_backslash_step1(x) for x in contents]
        
            with open(filename, 'w', encoding=self.__save_encoding) as f:
                for x in contents:
                    f.write(x)
            
            if self.__lang == 'zh-tw' and self.__big5_escape:
                convert_backslash_step2(filename)
            
            return True
        except Exception as _err:
            raise _err

    def execute(self, filename, savefile = None):
        if not Common.is_file_exists(filename):
            return False
        
        if not savefile:
            savefile = filename
        
        if not self.__silent:
            Message.ShowInfo('正在处理: %s (%s)' % (os.path.relpath(filename, self.__project_dir), self.__save_encoding))

        contents = self.__load(filename)
        contents = self.__process(contents)
        return self.__save(contents, savefile)

class FulltextReplaceController():
    def __init__(self, **kwargs):
        self.__id_pos = self.__getfromdict(kwargs, 'id_pos')
        self.__replace_pos = self.__getfromdict(kwargs, 'replace_pos')
        self.__pattern = self.__getfromdict(kwargs, 'pattern')
        self.__replace_sub = self.__getfromdict(kwargs, 'replace_sub')
        self.__save_encoding = self.__getfromdict(kwargs, 'save_encoding')
        self.__replace_escape = self.__getfromdict(kwargs, 'replace_escape')
        self.__project_dir = self.__getfromdict(kwargs, 'project_dir')
        self.__lang = self.__getfromdict(kwargs, 'lang')
        self.__silent = self.__getfromdict(kwargs, 'silent')
        self.__transdb_name = self.__getfromdict(kwargs, 'transdb_name')
        self.__replace_decorate = self.__getfromdict(kwargs, 'replace_decorate')
        self.__regex_flags = self.__getfromdict(kwargs, 'regex_flags', 0)
        self.__big5_escape = self.__getfromdict(kwargs, 'big5_escape', False)
        self.__trans = TranslateDatabase(self.__transdb_name, self.__lang)
    
    def __getfromdict(self, dictmap, key, default = None):
        if key not in dictmap:
            return default
        return dictmap[key]

    def __load(self, filename):
        # 将文件中的内容读取成一个完整的文本
        contents = None

        encoding = Common.get_file_encoding(filename)
        encoding = 'latin1' if encoding is None else encoding

        with open(filename, 'r', encoding=encoding) as f:
            contents = f.read()
        return contents
    
    def __escape(self, value):
        if value is None:
            return None

        escapelist = ['\'']
        escape_val = ''
        for c in value:
            if c in escapelist:
                escape_val += r'\\' +  c
            else:
                escape_val += c
        return escape_val

    def __process_sub(self, matched):
        mapid = matched.groups()[self.__id_pos - 1]

        if not mapid:
            return matched.group()
		
        transname = self.__trans.trans(mapid)

        if not transname:
            return matched.group()
        
        if self.__replace_escape:
            transname = self.__escape(transname)

        if self.__replace_decorate is not None:
            transname = globals()[self.__replace_decorate](
                matched.groups()[self.__replace_pos - 1], transname
            )
        
        replaced = self.__replace_sub
        replaced = replaced.format(trans = transname)

        for x, v in enumerate(matched.groups()):
            src = r'\g<{idx}>'.format(idx = x + 1)
            replaced = replaced.replace(src, v)

        return replaced

    def __process(self, contents):
        contents = re.sub(self.__pattern, self.__process_sub, contents, flags=self.__regex_flags)
        return contents
    
    def __save(self, contents, filename):
        try:
            if self.__lang == 'zh-tw' and self.__big5_escape:
                contents = convert_backslash_step1(contents)
        
            with open(filename, 'w', encoding=self.__save_encoding) as f:
                f.write(contents)
            
            if self.__lang == 'zh-tw' and self.__big5_escape:
                convert_backslash_step2(filename)

            return True
        except Exception as _err:
            raise _err

    def execute(self, filename, savefile = None):
        if not Common.is_file_exists(filename):
            return False
        
        if not savefile:
            savefile = filename
        
        if not self.__silent:
            Message.ShowInfo('正在处理: %s (%s)' % (os.path.relpath(filename, self.__project_dir), self.__save_encoding))

        contents = self.__load(filename)
        contents = self.__process(contents)
        return self.__save(contents, savefile)

class YamlReplaceController():
    def __init__(self, **kwargs):
        self.__id_field = self.__getfromdict(kwargs, 'id_field')
        self.__target_field = self.__getfromdict(kwargs, 'target_field')
        self.__target_pos = self.__getfromdict(kwargs, 'target_pos')
        self.__lang = self.__getfromdict(kwargs, 'lang')
        self.__transdb_name = self.__getfromdict(kwargs, 'transdb_name')
        self.__save_encoding = self.__getfromdict(kwargs, 'save_encoding')
        self.__silent = self.__getfromdict(kwargs, 'silent')
        self.__project_dir = self.__getfromdict(kwargs, 'project_dir')
        self.__big5_escape = self.__getfromdict(kwargs, 'big5_escape', False)
        self.__replace_escape = self.__getfromdict(kwargs, 'replace_escape')
        self.__replace_decorate = self.__getfromdict(kwargs, 'replace_decorate')
        self.__trans = TranslateDatabase(self.__transdb_name, self.__lang)

        self.__yaml=YAML(typ="rt")
        self.__yaml.default_flow_style = False
        self.__yaml.preserve_quotes = True
        self.__yaml.indent(mapping=2, sequence=4, offset=2)

    def __getfromdict(self, dictmap, key, default = None):
        if key not in dictmap:
            return default
        return dictmap[key]

    def __load(self, filename):
        contents = None

        encoding = Common.get_file_encoding(filename)
        encoding = 'latin1' if encoding is None else encoding

        with open(filename, 'r', encoding=encoding) as f:
            contents = self.__yaml.load(f.read())
        return contents

    def __escape(self, value):
        if value is None:
            return None

        escapelist = ['\'']
        escape_val = ''
        for c in value:
            if c in escapelist:
                escape_val += r'\\' +  c
            else:
                escape_val += c
        return escape_val

    def __process(self, contents):
        for item in contents['Body']:
            nameid = int(item[self.__id_field])
            transname = self.__trans.trans(nameid)

            if not transname:
                continue
            
            if self.__replace_escape:
                transname = self.__escape(transname)

            if self.__replace_decorate is not None:
                transname = globals()[self.__replace_decorate](item, transname)

            if self.__target_pos:
                if self.__target_field in item:
                    del item[self.__target_field]
                item.insert(self.__target_pos, self.__target_field, transname)
            else:
                item[self.__target_field] = transname
        return contents
    
    def __save(self, contents, filename):
        try:
            if self.__lang == 'zh-tw' and self.__big5_escape:
                contents = convert_backslash_step1(contents)
        
            with open(filename, 'w', encoding=self.__save_encoding) as f:
                self.__yaml.dump(contents, f)
            
            if self.__lang == 'zh-tw' and self.__big5_escape:
                convert_backslash_step2(filename)

            return True
        except Exception as _err:
            raise _err

    def execute(self, filename, savefile = None):
        if not Common.is_file_exists(filename):
            return False
        
        if not savefile:
            savefile = filename
        
        if not self.__silent:
            Message.ShowInfo('正在处理: %s (%s)' % (os.path.relpath(filename, self.__project_dir), self.__save_encoding))

        contents = self.__load(filename)
        contents = self.__process(contents)
        return self.__save(contents, savefile)

def process(project_dir, lang = 'zh-cn', with_doc = False, silent = False):
    try:
        # ===============================================================
        # 数据和 DB 文件翻译环节
        # ===============================================================
        Message.ShowStatus('正在根据对照表替换 db 中的物品、魔物、任务名称等...')
        for v in configures:
            operate_params = v['operate_params']
            operate_params['lang'] = lang
            operate_params['project_dir'] = project_dir
            operate_params['silent'] = silent
            operate = globals()[v['operate']](**operate_params)

            if 'filepath' in v:
                for path in v['filepath']:
                    fullpath = os.path.abspath(project_dir + path)
                    operate.execute(fullpath)
            
            if 'globpath' in v:
                for path in v['globpath']:
                    files = glob.glob(os.path.abspath(project_dir + path), recursive=True)
                    for x in files:
                        relpath = os.path.relpath(x, project_dir).replace('\\', '/')
                        is_exclude = False
                        if 'globexclude' in v:
                            for e in v['globexclude']:
                                if e in relpath:
                                    is_exclude = True
                                    break
                        if not is_exclude:
                            operate.execute(x)
        Message.ShowStatus('已完成对 db 文件的替换过程...')
        
        # ===============================================================
        # 配置和文档翻译环节 (使用 OpenCC 进行简繁转换)
        # ===============================================================
        if (lang == 'zh-tw' and with_doc):
            Message.ShowStatus('正在将文档中的简体中文转换成繁体中文...')
            for suffix in need_s2tw_convert:
                rule = project_dir + suffix

                for path in glob.glob(rule):
                    s2twp_convert(path)
            Message.ShowStatus('已完成对文档的转换过程...')

    except Exception as _err:
        Message.ShowError(str(_err))
        Common.exit_with_pause(-1)

def main():
    '''
    主入口函数
    '''
    Common.welcome('游戏文本转译辅助脚本')

    langtype = Inputer().requireSelect({
        'prompt' : '需要将文本翻译成的目标语言类型',
        'option_name' : '目标语言',
        'data' : [
            {
                'name' : '简体中文',
                'desc' : '0 - 汉化成简体中文'
            },
            {
                'name' : '繁体中文',
                'desc' : '1 - 汉化成繁体中文'
            }
        ]
    })

    with_doc = False
    if langtype == 1:   # zh-tw
        with_doc = Inputer().requireBool({
            'tips' : '是否也将文档 (conf, doc, etc...) 转换成繁体中文?',
            'default' : False
        })

    process(project_slndir, 'zh-cn' if langtype == 0 else 'zh-tw', with_doc)

    Common.exit_with_pause()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt as _err:
        pass
