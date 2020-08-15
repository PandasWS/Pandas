'''
//===== Pandas Python Script =================================
//= 源代码文件编码转换助手
//===== By: ================================================== 
//= Sola丶小克
//===== Current Version: ===================================== 
//= 1.0
//===== Description: ========================================= 
//= 此脚本用于将 src 目录中的源码文件都转换为 UTF-8-SIG 编码
//= 这里所谓的 UTF-8-SIG 实际上就是 UTF8-BOM 编码
//===== Additional Comments: ================================= 
//= 1.0 首个版本. [Sola丶小克]
//============================================================
'''

# -*- coding: utf-8 -*-

import codecs
import os

import chardet

from libs import Common, Inputer, Message


class CharsetConverter():
    '''
    此操作类用于帮助对文本文件进行编码转换
    '''
    def __init__(self, options):
        self.__options__ = options

    def __detectCharset(self, filepath):
        '''
        给定一个文件路径, 获取该文本文件的编码
        '''
        with open(filepath, 'rb') as hfile:
            return chardet.detect(hfile.read())['encoding']

    def convertDirectory(self, directory, to_charset):
        '''
        给定一个目录和期望的目标文本编码
        将该目录中的所有文件都转换成指定的编码
        '''
        ignore_files = self.__options__['ignore_files']
        process_exts = self.__options__['process_exts']

        ignore_files = [x.lower() for x in ignore_files]
        process_exts = [x.lower() for x in process_exts]

        convert_cnt = 0

        for dirpath, _dirnames, filenames in os.walk(directory):
            for filename in filenames:
                _base_name, extension_name = os.path.splitext(filename.lower())

                if filename.lower() in ignore_files:
                    continue
                if extension_name not in process_exts:
                    continue

                fullpath = os.path.normpath('%s/%s' % (dirpath, filename))
                if self.convertFile(fullpath, to_charset, directory):
                    convert_cnt = convert_cnt + 1

        return convert_cnt

    def convertFile(self, filepath, to_charset, directory):
        '''
        给定一个文本文件, 将它转换成指定的编码并保存
        '''
        origin_charset = self.__detectCharset(filepath).upper()
        to_charset = to_charset.upper()

        if origin_charset == to_charset:
            return False

        try:
            absolutely_path = os.path.abspath(filepath)

            Message.ShowInfo('将 {filename} 的编码从 {origin_charset} 转换为 {to_charset}'.format(
                filename=os.path.relpath(absolutely_path, directory),
                origin_charset=origin_charset,
                to_charset=to_charset
            ))

            origin_file = codecs.open(filepath, 'r', origin_charset)
            content = origin_file.read()
            origin_file.close()

            target_file = codecs.open(filepath, 'w', to_charset)
            target_file.write(content)
            target_file.close()

            return True
        except IOError as err:
            Message.ShowError("I/O error: {0}".format(err))
            return False

def main():
    os.chdir(os.path.split(os.path.realpath(__file__))[0])

    Common.welcome('源代码文件编码转换助手')
    print('')
	
    confirm = Inputer().requireBool({
        'tips' : '是否立刻进行文件编码转换?',
        'default' : False
    })
	
    if not confirm:
        Message.ShowInfo('您取消了操作, 程序终止...\n')
        Common.exit_with_pause()

    count = CharsetConverter({
        'ignore_files' : ['Makefile', 'Makefile.in', 'CMakeLists.txt'],
        'process_exts' : ['.hpp', '.cpp', '.h', '.c']
    }).convertDirectory('../../src', 'UTF-8-SIG')

    if count <= 0:
        Message.ShowInfo('很好! 源代码文件都已转换为 UTF-8-SIG 编码.')

    Common.exit_with_pause()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt as _err:
        pass
