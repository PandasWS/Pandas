# -*- coding: utf-8 -*-

import os
import re

import chardet

from libs import Common


class InjectController:
    def __init__(self, options):
        self.__options__ = options
        self.mark_dict = {}

        if not self.detect(self.__options__['source_dirs']):
            print('[状态] 无法成功定位所有需要的代码注入点, 程序终止!')
            Common.exitWithPause(-1)
        else:
            print('[状态] 已成功定位所有代码注入点.\n')

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

        if self.__detectCharset(filename).upper() != 'UTF-8-SIG':
            print('No UTF-8-SIG: %s' % (filename))
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
                    print('发现重复的代码注入标记: ' + re_match.group(0))
                    Common.exitWithPause(-1)

                self.mark_dict[re_match.group(1)] = {
                    'index' : int(re_match.group(1)),
                    'filepath' : filename,
                    'line' : line_num
                }
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
                self.__searchMark(fullpath)

        bMarkPassed = True
        for mark_index in range(1, self.__options__['mark_counts'] + 1):
            if str(mark_index) not in self.mark_dict:
                bMarkPassed = False
        return bMarkPassed

    def print(self):
        for _mark_index, mark_value in self.mark_dict.items():
            print('Insert Point {index} in File {filepath} at Line {line}'.format(
                index = mark_value['index'],
                filepath = mark_value['filepath'],
                line = mark_value['line']
            ))

    def insert(self, index, content):
        mark = self.mark_dict[str(index)]
        filepath = mark['filepath']
        insert_content = ('\n'.join(content)) + '\n'

        charset = self.__detectCharset(filepath)
        with open(filepath, encoding = charset) as rfile:
            filecontent = rfile.readlines()

        filecontent.insert(mark['line'] - 1, insert_content)

        with open(filepath, mode = 'w', encoding = charset) as wfile:
            wfile.writelines(filecontent)

        for mark_index, mark_value in self.mark_dict.items():
            if mark_index == index:
                continue

            if mark_value['filepath'].lower() != filepath.lower():
                continue

            if mark['line'] < mark_value['line']:
                mark_value['line'] = mark_value['line'] + len(content)
