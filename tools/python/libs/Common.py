# -*- coding: utf-8 -*-

import os
import platform
from libs import Message
from colorama import Back, Fore, Style, init

__all__ = [
	'exitWithPause',
	'welcome'
]

init()

def exitWithPause(exitcode = 0):
    if platform.system() == 'Windows':
        os.system('pause')
    exit(exitcode)

def welcome(scriptname = None):
    LINE_NORMAL = Back.RED + Style.BRIGHT
    LINE_GREENS = Back.RED + Style.BRIGHT + Fore.GREEN
    LINE_WHITED = Back.RED + Style.BRIGHT + Fore.WHITE
    LINE_ENDING = '\033[K' + Style.RESET_ALL

    print('')
    print(LINE_NORMAL + r"                                                                              " + LINE_ENDING)
    print(LINE_WHITED + r"                            Pandas Dev Team Presents                          " + LINE_ENDING)
    print(LINE_NORMAL + r"                     ____                    _                                " + LINE_ENDING)
    print(LINE_NORMAL + r"                    |  _ \  __ _  _ __    __| |  __ _  ___                    " + LINE_ENDING)
    print(LINE_NORMAL + r"                    | |_) |/ _` || '_ \  / _` | / _` |/ __|                   " + LINE_ENDING)
    print(LINE_NORMAL + r"                    |  __/| (_| || | | || (_| || (_| |\__ \                   " + LINE_ENDING)
    print(LINE_NORMAL + r"                    |_|    \__,_||_| |_| \__,_| \__,_||___/                   " + LINE_ENDING)
    print(LINE_NORMAL + r"                                                                              " + LINE_ENDING)
    print(LINE_GREENS + r"                               https://pandas.ws/                             " + LINE_ENDING)
    print(LINE_NORMAL + r"                                                                              " + LINE_ENDING)
    print(LINE_WHITED + r"               Pandas is only for learning and research purposes.             " + LINE_ENDING)
    print(LINE_WHITED + r"                      Please don't use it for commercial.                     " + LINE_ENDING)
    print(LINE_NORMAL + r"                                                                              " + LINE_ENDING)
    print('')
    if scriptname is not None:
        Message.ShowInfo('您现在启动的是: {scriptname}'.format(scriptname = scriptname))
    Message.ShowInfo('在使用此脚本之前, 建议确保 src 目录的工作区是干净的.')
    Message.ShowInfo('这样添加结果如果不符合预期, 可以轻松的利用 git 进行重置操作.')
