# -*- coding: utf-8 -*-

import os
import re

import chardet

from libs import Common, Message

__all__ = [
	'Injecter'
]

class Injecter:
    def __init__(self, options):
        self.__options__ = options
        self.mark_dict = {}
        self.is_pro = (False if 'is_pro' not in options else options['is_pro'])

        if not self.detect(self.__options__['source_dirs']):
            Message.ShowError('无法成功定位所有需要的代码注入点, 程序终止!')
            Common.exit_with_pause(-1)
        else:
            Message.ShowStatus('已成功定位所有代码注入点.\n')

    def __detectCharset(self, filepath):
        '''
        给定一个文件路径, 获取该文本文件的编码
        '''
        with open(filepath, 'rb') as hfile:
            return chardet.detect(hfile.read())['encoding']

    def __searchMark(self, filename):
        line_num = 0
        charset = self.__detectCharset(filename)
        regex = r'\s*?' + self.__options__['mark_format'] + r'\s*?'

        if '../../src/'.replace('/', os.path.sep) in filename and charset.upper() != 'UTF-8-SIG':
            Message.ShowWarning('期望文件 %s 的编码为 UTF-8-SIG 而实际上是 %s' % (filename, charset.upper()))
            return

        try:
            textfile = open(filename, encoding=charset)
            for line in textfile:
                line_num = line_num + 1
                if '//' not in line:
                    continue

                re_match = re.match(regex, line)
                if not re_match:
                    continue

                if str(re_match.group(1)) in self.mark_dict:
                    Message.ShowError('发现重复的代码注入标记: ' + re_match.group(0))
                    Common.exit_with_pause(-1)

                self.mark_dict[re_match.group(1)] = {
                    'index' : int(re_match.group(1)),
                    'filepath' : filename,
                    'line' : line_num
                }
            textfile.close()
        except Exception as err:
            Message.ShowError('文件 : %s | 错误信息 : %s' % (filename, err))

    def detect(self, src_dir):
        if isinstance(src_dir, str):
            src_dir = [src_dir]

        for single_dir in src_dir:
            for dirpath, _dirnames, filenames in os.walk(single_dir):
                for filename in filenames:
                    _base_name, extension_name = os.path.splitext(filename.lower())

                    if extension_name not in self.__options__['process_exts']:
                        continue

                    fullpath = os.path.normpath('%s/%s' % (dirpath, filename))
                    self.__searchMark(fullpath)

        bMarkPassed = True
        for configure in self.__options__['mark_configure']:
            mark_id = int(configure['id'])
            if self.is_pro and ('pro_offset' in configure):
                mark_id += int(configure['pro_offset'])
            if str(mark_id) not in self.mark_dict:
                Message.ShowError('无法定位代码注入点: {index} : {constant} : {desc}'.format(
                    index = str(mark_id),
                    constant = str(configure['id']),
                    desc = configure['desc']
                ))
                bMarkPassed = False
        return bMarkPassed

    def print(self):
        for _mark_index, mark_value in self.mark_dict.items():
            Message.ShowDebug('代码注入点 {index} : {constant} 位于文件 {filepath} : {line} 行.'.format(
                index = '%-2s' % mark_value['index'],
                constant = '%-40s' % str(self.__options__['mark_enum'](mark_value['index'])),
                filepath = mark_value['filepath'],
                line = mark_value['line']
            ))

    def insert(self, index, content):
        mark_id = int(index)
        
        if self.is_pro:
            for configure in self.__options__['mark_configure']:
                if int(configure['id']) == mark_id:
                    if 'pro_offset' in configure:
                        mark_id += int(configure['pro_offset'])
        
        mark = self.mark_dict[str(mark_id)]
        filepath = mark['filepath']
        insert_content = ('\n'.join(content)) + '\n'

        charset = self.__detectCharset(filepath)
        with open(filepath, encoding = charset) as rfile:
            filecontent = rfile.readlines()

        filecontent.insert(mark['line'] - 1, insert_content)

        with open(filepath, mode = 'w', encoding = charset) as wfile:
            wfile.writelines(filecontent)

        for mark_index, mark_value in self.mark_dict.items():
            if mark_index == mark_id:
                continue

            if mark_value['filepath'].lower() != filepath.lower():
                continue

            if mark['line'] < mark_value['line']:
                mark_value['line'] = mark_value['line'] + len(content)
