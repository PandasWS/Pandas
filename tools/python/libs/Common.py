# -*- coding: utf-8 -*-

import binascii
import glob
import os
import platform
import re
import shutil
import subprocess
import time

import git
import pdbparse
import pefile
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
	'welcome',
    'timefmt',
    'get_pandas_ver',
    'get_pandas_branch',
    'get_pandas_hash',
    'match_file_regex',
    'is_compiled',
    'get_pe_hash',
    'get_pdb_hash',
    'get_file_ext'
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
        message = cmdProc.stdout.readline()
        if not message and cmdProc.poll() is not None:
            break
        if message and logfile is not None:
            logger.write(message)
            logger.flush()
        if message:
            message = '{prefix}{value}'.format(
                prefix = '%s: ' % prefix if prefix is not None else '',
                value = message.strip()
            )
            Message.ShowInfo(message)
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
        print('')
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

def timefmt(compact = False):
    '''
    返回当前时间的字符串格式
    当 compact 为 True 的时候返回紧凑格式
    '''
    fmt = '%Y%m%d_%H%M%S' if compact else '%Y-%m-%d %H:%M:%S'
    return time.strftime(fmt, time.localtime(time.time()))

def match_file_regex(filename, pattern, encoding = 'UTF-8-SIG'):
    '''
    在一个文件中找出符合正则表达式匹配的结果集合
    该函数被设计成找到第一个结果就会返回, 且会将内容尽量转换成 list 而不是 tuple
    若找不到匹配的结果则返回 None
    '''
    with open(filename, 'r', encoding = encoding) as f:
        lines = f.readlines()
        f.close()
    
    for line in lines:
        regexGroup = re.findall(pattern, line)
        if regexGroup:
            return list(regexGroup[0]) if isinstance(regexGroup[0], tuple) else regexGroup
    return None

def get_pandas_ver(slndir, prefix = None):
    '''
    读取当前 Pandas 在 src/config/pandas.hpp 定义的版本号
    若读取不到版本号则返回 None
    '''
    filepath = os.path.abspath(slndir + '/src/config/pandas.hpp')
    if not is_file_exists(filepath):
        return None
    matchgroup = match_file_regex(filepath, r'#define Pandas_Version "(.*)"')
    version = matchgroup[0] if matchgroup is not None else None
    return version if not prefix else prefix + version

def get_pandas_branch(slndir):
    '''
    获取当前代码仓库的分支名称
    '''
    repo = git.Repo(slndir)
    return str(repo.active_branch)

def get_pandas_hash(slndir):
    '''
    获取当前代码仓库的 HASH 版本号
    '''
    repo = git.Repo(slndir)
    return repo.head.object.hexsha

def is_compiled(slndir, checkmodel = 'all', checksymbol = True):
    '''
    检查编译产物是否存在, 可以控制判范围
    
    checkmodel:
        all 表示检查复兴前和复兴后的编译产物
        re 表示只检查复兴后的编译产物
        pre 表示只检查复兴前的编译产物
    
    checksymbol:
        True 表示需要同时检查符号文件
        False 表示不检查符号文件
    '''
    checkfiles = []
    
    if 're' == checkmodel or 'all' == checkmodel:
        checkfiles.append('login-server.exe')
        checkfiles.append('char-server.exe')
        checkfiles.append('map-server.exe')
        checkfiles.append('csv2yaml.exe')
        
        if checksymbol:
            checkfiles.append('login-server.pdb')
            checkfiles.append('char-server.pdb')
            checkfiles.append('map-server.pdb')
            checkfiles.append('csv2yaml.pdb')
        
    if 'pre' == checkmodel or 'all' == checkmodel:
        checkfiles.append('login-server-pre.exe')
        checkfiles.append('char-server-pre.exe')
        checkfiles.append('map-server-pre.exe')
        checkfiles.append('csv2yaml.exe')
        
        if checksymbol:
            checkfiles.append('login-server-pre.pdb')
            checkfiles.append('char-server-pre.pdb')
            checkfiles.append('map-server-pre.pdb')
            checkfiles.append('csv2yaml.pdb')

    for filename in checkfiles:
        filepath = os.path.abspath(slndir + filename)
        if not is_file_exists(filepath):
            return False
    return True

def get_pe_hash(pefilepath):
    '''
    获取 PE 文件的哈希值
    '''
    if not is_file_exists(pefilepath):
        return None
    pe = pefile.PE(pefilepath, fast_load=True)
    return '%X%X' % (pe.FILE_HEADER.TimeDateStamp, pe.OPTIONAL_HEADER.SizeOfImage)

def get_pdb_hash(pdbfilepath):
    '''
    获取 PDB 文件的哈希值
    '''
    if not is_file_exists(pdbfilepath):
        return None
    p = pdbparse.parse(pdbfilepath, fast_load = True)
    pdb = p.streams[pdbparse.PDB_STREAM_PDB]
    pdb.load()
    guidstr = (u'%08x%04x%04x%s%x' % (pdb.GUID.Data1, pdb.GUID.Data2, pdb.GUID.Data3, binascii.hexlify(
        pdb.GUID.Data4).decode('ascii'), pdb.Age)).upper()
    return guidstr

def get_file_ext(filepath):
    '''
    获取文件的后缀名, 包括小数点例如: .exe
    '''
    _base_name, extension_name = os.path.splitext(filepath.lower())
    return extension_name
