# -*- coding: utf-8 -*-

import binascii
import codecs
import glob
import hashlib
import os
import platform
import re
import shutil
import subprocess
import time
from functools import partial

import chardet
import git
import pefile
from colorama import Back, Fore, Style, init

from libs import Message

if platform.system() == 'Windows':
    import winreg
    import pdbparse

__all__ = [
    'md5',
    'glob_delete',
    'cmd_execute',
    'read_regstr',
    'is_file_exists',
    'is_dir_exists',
	'exit_with_pause',
	'welcome',
    'timefmt',
    'get_community_ver',
    'get_commercial_ver',
    'get_pandas_ver',
    'get_pandas_branch',
    'get_pandas_hash',
    'match_file_regex',
    'match_file_resub',
    'get_encoding',
    'is_compiled',
    'is_pandas_release',
    'get_pe_hash',
    'get_pdb_hash',
    'get_file_ext',
    'get_byte_encoding',
    'get_file_encoding',
    'normalize_path',
    'display_version_info',
    'get_current_branch',
    'is_jenkins'
]

init()

def md5(value):
    '''
    获取字符串的 md5 哈希散列
    '''
    return hashlib.md5(value.encode(encoding='UTF-8')).hexdigest()

def glob_delete(glob_pattern, excludes = None):
    '''
    删除一个指定通配符路径中的全部文件
    '''
    contents = glob.glob(glob_pattern)
    for path in contents:
        if excludes and str.lower(os.path.basename(path)) in excludes:
            continue
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


def encoding_maps(encoding):
    '''
    对一些编码的超集进行转换,
    比如 CP949 是 EUC-KR 的超集, GBK 是 GB2312 的超集
    '''
    encodeingMaps = {
        'EUC-KR': 'CP949',
        'GB2312': 'GBK'
    }

    if encoding.upper() in encodeingMaps:
        encoding = encodeingMaps[encoding.upper()]

    return encoding

def get_byte_encoding(bytebuffer):
    if bytebuffer is not None:
        response = chardet.detect(bytebuffer)
        encoding = response['encoding']
        return None if encoding is None else encoding_maps(encoding)
    return None

def get_file_encoding(filepath):
    def analysis_blocks_encoding(blocks_encoding):
        validEncoding = {}

        for encoding in blocks_encoding:
            # 检测不到编码内容, 或者概率小于 50% 的过滤掉
            if encoding is None or encoding['confidence'] < 0.5:
                continue
            # 记录这个编码出现的次数, 后续推断使用
            codepage = encoding['encoding']
            if codepage is not None:
                codepage = codepage.upper()
            codepage = encoding_maps(codepage)
            if codepage not in validEncoding:
                validEncoding[codepage] = 1
            else:
                validEncoding[codepage] = validEncoding[codepage] + 1

        validEncodingList = sorted(validEncoding, reverse=True)

        if 'CP949' in validEncodingList or 'EUC-KR' in validEncodingList:
            validEncodingList.append('GBK')
            validEncodingList.append('LATIN1')

        return validEncodingList

    def try_useing_encoding(filepath, encoding):
        try:
            encoding = encoding_maps(encoding)
            f = open(filepath, 'r', encoding = encoding)
            f.readlines()
            f.close()
            return True
        except UnicodeDecodeError as _err:
            return False

    f = open(filepath, 'rb')

    # 获取文件的总长度
    f.seek(0, 2)
    filesize = f.tell()
    f.seek(0, 0)

    # 初始化的块长度为 1024 * 8
    blocksize = 1024 * 8

    # 文件大小和块大小相比, 取最小那个值, 作为新的块大小
    blocksize = filesize if filesize < blocksize else blocksize

    # 每次读取 blocksize 长度的数据
    blocks = iter(partial(f.read, blocksize), b'')

    # 记录每个块的编码检测结果, 和判断准确率
    blocks_encoding = []
    failed_encoding = []

    for block in blocks:
        response = chardet.detect(block)
        if response is not None and \
            response['encoding'] not in failed_encoding:
            blocks_encoding.append(response)

        # 当获得的块结果小于 10 次, 那么继续分析
        if len(blocks_encoding) < 10:
            continue

        # 如果大于等于 10 次, 那么对内容进行一次排重整理
        validEncodingList = analysis_blocks_encoding(blocks_encoding)
        vaildEncoding = None

        for encoding in validEncodingList:
            if try_useing_encoding(filepath, encoding):
                vaildEncoding = encoding_maps(encoding)
                break
            else:
                failed_encoding.append(encoding)

        if vaildEncoding is not None:
            return vaildEncoding
        blocks_encoding.clear()

    if blocks_encoding is not None:
        validEncodingList = analysis_blocks_encoding(blocks_encoding)
        for encoding in validEncodingList:
            if try_useing_encoding(filepath, encoding):
                return encoding_maps(encoding)

    f.close()
    return None


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
    Message.ShowInfo('这样当处理结果不符合预期时, 可以轻松的利用 git 进行重置操作.')

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

def match_file_resub(filename, pattern, repl, encoding = None):
    '''
    在一个文件中找出符合正则表达式匹配的结果集合, 并进行替换操作
    '''
    if encoding is None:
        encoding = get_encoding(filename)

    with open(filename, 'r', encoding = encoding) as f:
        lines = f.readlines()
        f.close()
    
    for idx, value in enumerate(lines):
        lines[idx] = re.sub(pattern, repl, value)
    
    with open(filename, 'wt', encoding = encoding) as f:
        f.writelines(lines)
        f.close()

def get_encoding(filename):
    '''
    获取打开此文件所需要的编码方式
    '''
    with open(filename, 'rb') as f:
        data = f.read()
        result = chardet.detect(data)
        f.close()
    
    if str.lower(result['encoding']) == 'utf-16':
        if data.startswith(codecs.BOM_LE):
            return 'utf-16-le'
        elif data.startswith(codecs.BOM_BE):
            return 'utf-16-be'

    return str.lower(result['encoding'])

def get_community_ver(slndir, prefix = None, origin = False):
    '''
    读取当前 Pandas 在 src/config/pandas.hpp 定义的社区版版本号
    若读取不到版本号则返回 None
    '''
    filepath = os.path.abspath(slndir + '/src/config/pandas.hpp')
    if not is_file_exists(filepath):
        return None
    matchgroup = match_file_regex(filepath, r'#define Pandas_Version "(.*)"')
    version = matchgroup[0] if matchgroup is not None else None
    
    if not origin:
        # 读取到的版本号应该是四段式的
        # 将其加工成三段式, 并在末尾追加可能需要的 -dev 后缀
        splitver = version.split('.')
        if (len(splitver) == 4):
            version = '%s.%s.%s' % (splitver[0], splitver[1], splitver[2])
            if splitver[3] == '1':
                version += '-dev'
    
    return version if not prefix else prefix + version


def get_commercial_ver(slndir, prefix = None, origin = False):
    '''
    读取当前 Pandas 在 src/config/professional.hpp 定义的商业版版本号
    若读取不到版本号则返回 None
    '''
    filepath = os.path.abspath(slndir + '/src/config/professional.hpp')
    if not is_file_exists(filepath):
        return None
    matchgroup = match_file_regex(filepath, r'#define Pandas_Commercial_Version "(.*)"')
    version = matchgroup[0] if matchgroup is not None else None
    
    if not origin:
        # 读取到的版本号应该是四段式的
        # 将其加工成三段式, 并在末尾追加可能需要的修订后缀
        splitver = version.split('.')
        if (len(splitver) == 4):
            version = '%s.%s.%s' % (splitver[0], splitver[1], splitver[2])
            if splitver[3] != '0':
                version += ' Rev.%s' % splitver[3]
    
    return version if not prefix else prefix + version


def is_commercial_ver(slndir):
    '''
    当前是否为专业版 (商业版)
    '''
    filepath = os.path.abspath(slndir + '/src/config/professional.hpp')
    if not is_file_exists(filepath):
        return False
    matchgroup = match_file_regex(filepath, r'#define Pandas_Commercial_Version "(.*)"')
    return matchgroup is not None


def get_pandas_ver(slndir, prefix = None, origin = False):
    '''
    根据是否启用了专业版 (商业版) 来获得对应的展示版本
    '''
    if is_commercial_ver(slndir):
        return get_commercial_ver(slndir, prefix, origin)
    else:
        return get_community_ver(slndir, prefix, origin)


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
        checkfiles.append('yaml2sql.exe')
        checkfiles.append('yamlupgrade.exe')
        
        if checksymbol:
            checkfiles.append('login-server.pdb')
            checkfiles.append('char-server.pdb')
            checkfiles.append('map-server.pdb')
            checkfiles.append('csv2yaml.pdb')
            checkfiles.append('yaml2sql.pdb')
            checkfiles.append('yamlupgrade.pdb')
        
    if 'pre' == checkmodel or 'all' == checkmodel:
        checkfiles.append('login-server-pre.exe')
        checkfiles.append('char-server-pre.exe')
        checkfiles.append('map-server-pre.exe')
        checkfiles.append('csv2yaml-pre.exe')
        checkfiles.append('yaml2sql-pre.exe')
        checkfiles.append('yamlupgrade-pre.exe')
        
        if checksymbol:
            checkfiles.append('login-server-pre.pdb')
            checkfiles.append('char-server-pre.pdb')
            checkfiles.append('map-server-pre.pdb')
            checkfiles.append('csv2yaml-pre.pdb')
            checkfiles.append('yaml2sql-pre.pdb')
            checkfiles.append('yamlupgrade-pre.pdb')

    for filename in checkfiles:
        filepath = os.path.abspath(slndir + filename)
        if not is_file_exists(filepath):
            return False
    return True

def is_pandas_release(slndir):
    '''
    读取当前 Pandas 在 src/config/pandas.hpp 定义的版本号
    并判断最后一段是否为 0 (表示正式版)
    '''
    # 若是专业版则认为本次编译的就是正式版
    if is_commercial_ver(slndir):
        return True
    
    # 若是社区版则根据版本后最后一位进行判断
    version = get_community_ver(slndir, origin = True)
    splitver = version.split('.')
    return (len(splitver) == 4 and splitver[3] == '0')

def get_pe_hash(pefilepath):
    '''
    获取 PE 文件的哈希值
    '''
    if not is_file_exists(pefilepath):
        return None
    pe = pefile.PE(pefilepath, fast_load=True)
    return '%X%x' % (pe.FILE_HEADER.TimeDateStamp, pe.OPTIONAL_HEADER.SizeOfImage)

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

def normalize_path(path):
    '''
    将路径中的斜杠统一为反斜杠
    '''
    return path.replace('/', os.path.sep).replace('\\', os.path.sep)

def display_version_info(slndir_path, repo = None):
    '''
    显示 Pandas 的版本信息
    '''
    pandas_communtiy_ver = get_community_ver(slndir_path, origin=True)
    pandas_ver = get_community_ver(slndir_path, prefix='v', origin=False)
    if is_commercial_ver(slndir_path):
        pandas_commercial_ver = get_commercial_ver(slndir_path, origin=True)
        pandas_display_ver = get_pandas_ver(slndir_path, prefix='v')
        Message.ShowInfo('社区版版本号: %s | 专业版版本号: %s' % (pandas_communtiy_ver, pandas_commercial_ver))
        Message.ShowInfo('最终显示版本: %s' % pandas_display_ver)
    else:
        Message.ShowInfo('社区版版本号: %s (%s)' % (pandas_communtiy_ver, pandas_ver))
    
    if repo and get_current_branch(repo):
        Message.ShowInfo('当前代码分支: %s' % get_current_branch(repo))

def get_current_branch(repo):
    '''
    获取当前仓库的分支名称
    '''
    branch_name = None
    try:
        branch_name = repo.active_branch.name
    except Exception as _err:
        pass
    return branch_name

def is_jenkins():
    '''
    判断当前是否在 Jenkins 上运行
    '''
    return 'JENKINS_HOME' in os.environ
