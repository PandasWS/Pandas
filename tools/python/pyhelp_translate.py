'''
//===== Pandas Python Script ============================== 
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

import os
import re
from libs import Common, Message, Inputer

# 切换工作目录为脚本所在目录
os.chdir(os.path.split(os.path.realpath(__file__))[0])

# 工程文件的主目录相对此脚本文件的位置
project_slndir = '../../'

# 配置需要进行自动汉化处理的相关信息
configures = [
    {
        'operate' : 'ReplaceController',
        'operate_params' : {
            'transdb_name' : 'itemname',
            'pattern' : r'(//|)(\d+)(,.*?,)(.*?)(,.*?)$',
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>\g<5>',
            'id_pos' : 2,
            'replace_pos' : 4
        },
        'filepath' : [
            'db/re/item_db.txt',
            'db/pre-re/item_db.txt'
        ]
    },
    {
        'operate' : 'ReplaceController',
        'operate_params' : {
            'transdb_name' : 'mobname',
            'pattern' : r'(//|)(\d+)(,.*?,)(.*?)(,.*?)$',
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>\g<5>',
            'id_pos' : 2,
            'replace_pos' : 4
        },
        'filepath' : [
            'db/re/mob_db.txt',
            'db/pre-re/mob_db.txt'
        ]
    },
    {
        'operate' : 'ReplaceController',
        'operate_params' : {
            'transdb_name' : 'mobname',
            'pattern' : r'(//|)(.*?,)(\d+)(,.*?)(.*?)(,.*)',
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>\g<5>\g<6>',
            'id_pos' : 3,
            'replace_pos' : 5
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
        'operate' : 'ReplaceController',
        'operate_params' : {
            'transdb_name' : 'itemname',
            'pattern' : r'(//.*?|.*?)(\d+)(.*?//)(.*)',
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>',
            'id_pos' : 2,
            'replace_pos' : 4
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
        'operate' : 'ReplaceController',
        'operate_params' : {
            'transdb_name' : 'itemname',
            'pattern' : r'(//.*?,|.*?,)(\d+)(.*?//)(.*)',
            'replace_sub' : r'\g<1>\g<2>\g<3>\g<4>',
            'id_pos' : 2,
            'replace_pos' : 4
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
    }
]

class TranslateDatabase():
    def __init__(self, name, lang = 'zh-cn'):
        self.__loaded = False
        self.__translateDict = {}
        self.__load(os.path.abspath('./database/%s/%s.txt' % (lang, name)))
    
    def __load(self, filename):
        if not os.path.exists(filename):
            return "文件不存在"
        
        if not os.path.isfile(filename):
            return "大哥你给我个文件行不"

        # 将文件中的内容读取成一个列表, 放在 contents 中
        contents = []
        with open(filename, 'r', encoding='UTF-8-SIG') as f:
            contents = f.readlines()

        # 遍历处理列表中的全部内容, 让他们每一行都去掉左右的空格
        contents = [x.strip() for x in contents]

        # 移除内容中有反斜杠的行
        contents = [x for x in contents if not x.startswith('//')]

        # 把列表中的内容加工成字典, 若 ID 冲突则以最后读取到的为准
        for line in contents:
            fields = line.split(',')

            if len(fields) != 2:
                continue
            
            if not fields[0].isdigit():
                continue

            if not fields[1]:
                continue

            mapid = int(fields[0])
            self.__translateDict[mapid] = fields[1]
        self.__loaded = True
    
    def trans(self, id):
        if not self.__loaded:
            raise Exception("大哥你还没初始化就调我干啥")

        if int(id) in self.__translateDict:
            return self.__translateDict[int(id)]

        return None

class ReplaceController():
    def __init__(self, transdb_name, pattern, replace_sub, id_pos, replace_pos, lang):
        self.__id_pos = id_pos
        self.__replace_pos = replace_pos
        self.__pattern = pattern
        self.__replace_sub = replace_sub
        self.__trans = TranslateDatabase(transdb_name, lang)
    
    def __load(self, filename):
        # 将文件中的内容读取成一个列表, 放在 contents 中
        contents = []
        with open(filename, 'r', encoding='UTF-8-SIG') as f:
            contents = f.readlines()
        return contents
    
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

            repl = self.__replace_sub
            repl = repl.replace('\\g<%d>' % self.__replace_pos, transname)
            contents[k] = re.sub(self.__pattern, repl, line)
        return contents

    def __save(self, contents, filename):
        try:
            with open(filename, 'w', encoding='UTF-8-SIG') as f:
                for x in contents:
                    f.write(x)
            return True
        except Exception as _err:
            return False

    def execute(self, filename, savefile = None):
        if not Common.is_file_exists(filename):
            return False
        
        if not savefile:
            savefile = filename
        
        contents = self.__load(filename)
        contents = self.__process(contents)
        return self.__save(contents, savefile)



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

    for v in configures:
        operate_params = v['operate_params']
        operate_params['lang'] = 'zh-cn' if langtype == 0 else 'zh-tw'
        operate = globals()[v['operate']](**operate_params)
        for path in v['filepath']:
            fullpath = os.path.abspath(project_slndir + path)
            operate.execute(fullpath)

    Common.exit_with_pause()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt as _err:
        pass