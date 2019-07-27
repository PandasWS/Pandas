# -*- coding: utf-8 -*-

import glob
import os
import platform
import shutil
import subprocess
import time

from colorama import Back, Fore, Style, init

from libs import Message

if platform.system() == 'Windows':
    import winreg

__all__ = [
    'glob_delete',
    'cmd_execute',
    'read_regstr',
    'is_file_exists',
    'is_dir_exists',
	'exit_with_pause',
	'welcome'
]

init()

def glob_delete(glob_pattern):
    '''
    删除一个指定通配符路径中的全部文件
    '''
    contents = glob.glob(glob_pattern)
    for path in contents:
        if os.path.isfile(path):
            os.remove(path)
            if not os.listdir(os.path.dirname(path)):
                os.rmdir(os.path.dirname(path))
        if os.path.isdir(path):
            shutil.rmtree(path, ignore_errors=True)

def cmd_execute(cmds, cwd, prefix = None, logfile = None):
    '''
    在指定的 cwd 工作目录下执行一系列 cmd 指令
    并将执行的结果记录带 logfile 文件中
    '''
    if platform.system() != 'Windows':
        print('被调用的 cmd_execute 函数只能在 Windows 环境上运行')
        exit_with_pause(-1)

    if logfile is not None:
        os.makedirs(os.path.dirname(logfile), exist_ok=True)
        logger = open(logfile, 'w', encoding='utf8')
    
    cmdline = ' && '.join(cmds)
    cmdProc = subprocess.Popen(r'cmd /c "%s"' % cmdline,
        stdout = subprocess.PIPE, universal_newlines=True,
        cwd=os.path.abspath(cwd)
    )

    while True:
        stdout_data = cmdProc.stdout.readline()
        if not stdout_data and cmdProc.poll() is not None:
            break
        if stdout_data:
            Message.ShowInfo('%s%s' % ('%s: ' % prefix if prefix is not None else '', stdout_data.strip()))
        if stdout_data and logfile is not None:
            logger.write(stdout_data)
            logger.flush()
        time.sleep(0.01)
    
    if logfile is not None:
        logger.close()
    return cmdProc.returncode

def read_regstr(key, subkey, name):
    '''
    从注册表读取指定的字段值并返回
    指定的路径或者键值不存在的话, 则返回 None
    '''
    try:
        regKey = winreg.OpenKey(key, subkey)
        regValue, _regType = winreg.QueryValueEx(regKey, name)
        return regValue
    except FileNotFoundError as _err:
        return None

def is_file_exists(filepath):
    '''
    判断一个文件是否存在
    '''
    return (os.path.exists(filepath) and os.path.isfile(filepath))

def is_dir_exists(dirpath):
    '''
    判断一个目录是否存在
    '''
    return (os.path.exists(dirpath) and os.path.isdir(dirpath))

def exit_with_pause(exitcode = 0):
    '''
    终止脚本, 若是在 Windows 平台下则退出之前暂停一下
    以便用户看清楚程序提示的内容
    '''
    if platform.system() == 'Windows':
        os.system('pause')
    exit(exitcode)

def welcome(scriptname = None):
    '''
    用于在脚本程序开始时显示欢迎信息
    '''
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
