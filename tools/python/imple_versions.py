'''
//===== Pandas Python Script =================================
//= 版本号批量修改辅助脚本
//===== By: ================================================== 
//= Sola丶小克
//===== Current Version: ===================================== 
//= 1.0
//===== Description: ========================================= 
//= 此脚本用于快速修改当前工程的版本号
//===== Additional Comments: ================================= 
//= 1.0 首个版本. [Sola丶小克]
//============================================================
'''

# -*- coding: utf-8 -*-

import environment
environment.initialize()

import os
import re

from libs import Common, Inputer, Message

# 切换工作目录为脚本所在目录
os.chdir(os.path.split(os.path.realpath(__file__))[0])

# 工程文件的主目录相对此脚本文件的位置
project_slndir = '../../'

options = {
    r'.*\.rc$' : [
        {
            'regex' : r'FILEVERSION (\d+,\d+,\d+,\d+)',
            'replto' : r'FILEVERSION %s',
            'vmode' : ','
        },
        {
            'regex' : r'PRODUCTVERSION (\d+,\d+,\d+,\d+)',
            'replto' : r'PRODUCTVERSION %s',
            'vmode' : ','
        },
        {
            'regex' : r'VALUE "FileVersion", "(.*)"',
            'replto' : r'VALUE "FileVersion", "%s"',
            'vmode' : '.'
        },
        {
            'regex' : r'VALUE "ProductVersion", "(.*)"',
            'replto' : r'VALUE "ProductVersion", "%s"',
            'vmode' : '.'
        }
    ],
    r'^pandas\.hpp$' : [
        {
            'regex' : r'#define Pandas_Version "(.*)"',
            'replto' : r'#define Pandas_Version "%s"',
            'vmode' : '.',
            'for_type' : 0
        },
        {
            'regex' : r'#define Pandas_Commercial_Version "(.*)"',
            'replto' : r'#define Pandas_Commercial_Version "%s"',
            'vmode' : '.',
            'for_type' : 1
        },
    ]
}

def slndir(path):
    '''
    基于工程文件根目录进行相对路径的转换
    '''
    return os.path.abspath('%s/%s' % (project_slndir, path))

def isVersionFormatValid(ver):
    '''
    判断版本号的格式是否正确 (四段式, 逗号分割)
    '''
    if not '.' in ver:
        return False
    fields = ver.split('.')
    if len(fields) != 4:
        return False
    for field in fields:
        if not field.isdigit():
            return False
    return True

class VersionUpdater():
    def __init__(self, options):
        self.__options__ = options
        self.__lastdir__ = ''
    
    def versionFormat(self, version, vmode):
        ver_fields = []
        if '.' in version:
            ver_fields = version.split('.')
        elif ',' in version:
            ver_fields = version.split(',')
        else:
            Message.ShowError('您传入的版本号格式不正确: %s' % version)
            Common.exit_with_pause(-1)

        if vmode == '.':
            return '.'.join(ver_fields)
        elif vmode == ',':
            return ','.join(ver_fields)
        elif vmode == 'fmt':
            return 'v%s.%s.%s%s' % (
                ver_fields[0], ver_fields[1], ver_fields[2],
                '-dev' if ver_fields[3] == '1' else ''
            )
        else:
            Message.ShowError('您传入的 vmode 版本号格式不正确: %s' % vmode)
            Common.exit_with_pause(-1)

        return version

    def processFile(self, filename, rules, version, ver_type):
        displaypath = filename if self.__lastdir__ == '' else os.path.relpath(filename, os.path.join(self.__lastdir__, '../'))
        Message.ShowStatus('正在更新 %s 中的版本号到: %s' % (displaypath, version))
        for rule in rules:
            if 'for_type' in rule and rule['for_type'] != ver_type:
                continue
            usever = self.versionFormat(version, rule['vmode'])
            replto = rule['replto'] % usever
            Common.match_file_resub(filename, rule['regex'], replto)

    def updateDirectory(self, directory, version, ver_type):
        self.__lastdir__ = directory

        fregexs = [v for _x, v in enumerate(self.__options__)]

        for dirpath, _dirnames, filenames in os.walk(directory):
            for filename in filenames:
                fullpath = os.path.normpath(os.path.join(dirpath, filename))
                for fregex in fregexs:
                    if re.match(fregex, filename) is None:
                        continue
                    rules = self.__options__[fregex]
                    self.processFile(fullpath, rules, version, ver_type)

def main():
    os.chdir(os.path.split(os.path.realpath(__file__))[0])

    Common.welcome('版本号批量修改辅助脚本')

    print('')

    # 判断当前是否为专业版
    slndir_path = os.path.abspath(project_slndir)
    is_commercial = Common.is_commercial_ver(slndir_path)

    # 读取当前熊猫模拟器的版本号
    pandas_communtiy_ver = Common.get_community_ver(slndir_path, origin=True)
    pandas_ver = Common.get_community_ver(slndir_path, prefix='v', origin=False)
    if is_commercial:
        pandas_commercial_ver = Common.get_commercial_ver(slndir_path, origin=True)
        pandas_display_ver = Common.get_pandas_ver(slndir_path, prefix='v')
        Message.ShowInfo('社区版版本号: %s | 专业版版本号: %s' % (pandas_communtiy_ver, pandas_commercial_ver))
        Message.ShowInfo('最终显示版本: %s' % pandas_display_ver)
    else:
        Message.ShowInfo('社区版版本号: %s (%s)' % (pandas_communtiy_ver, pandas_ver))

    # 确认当前的版本号规范
    rule_tips = '格式: 1.0.0.1, 第四段为 1 则表示为开发版'
    if is_commercial:
        rule_tips = '格式: YYYY.MM.DD.REV, 第四段为修订版号, 从 0 开始'

    # 询问获取升级后的目标版本
    newver = Inputer().requireText({
        'tips' : '请输入新的版本号 (%s)' % rule_tips,
    })

    if not isVersionFormatValid(newver):
        Message.ShowError('你输入的版本号: %s 不符合四段式规则, 请重试...' % newver)
        Common.exit_with_pause(-1)

    # 执行更新操作
    ver = VersionUpdater(options)
    ver.updateDirectory(slndir('src'), newver, 1 if is_commercial else 0)

    Common.exit_with_pause()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt as _err:
        pass
