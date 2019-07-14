'''
//===== Pandas Python Script ============================== 
//= 编译流程辅助脚本
//===== By: ================================================== 
//= Sola丶小克
//===== Current Version: ===================================== 
//= 1.0
//===== Description: ========================================= 
//= 此脚本用于编译 Release 版本的 Pandas 并储存相关符号文件
//===== Additional Comments: ================================= 
//= 1.0 首个版本. [Sola丶小克]
//============================================================
'''

# -*- coding: utf-8 -*-

import os
import platform
import re
import shutil
import time
import winreg

import pefile
import pdbparse
import binascii

import git

from libs import Common, Inputer, Message

# 切换工作目录为脚本所在目录
os.chdir(os.path.split(os.path.realpath(__file__))[0])

# 工程文件的主目录相对此脚本文件的位置
project_slndir = '../../'

# 此脚本所需要的缓存目录 (缓存符号文件, 编译输出日志)
project_cachedir = 'tools/python/cache/'

# 符号仓库工程路径
project_symstoredir = os.path.abspath(project_slndir + '../symbols')

# 符号文件的 Git 仓库路径
symbols_giturl = 'https://github.com/PandasWS/Symbols.git'

# 编译输出日志的路径
compile_logfile = '%s/compile.log' % project_cachedir

# 配置能支持的 Visual Studio 相关信息
vs_configure = [
    {
        'name' : 'Visual Studio 2019',
        'reg_root' : winreg.HKEY_LOCAL_MACHINE,
        'reg_subkey' : r'SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\{tag}',
        'reg_key' : 'InstallLocation',
        'vcvarsall' : r'VC\Auxiliary\Build\vcvarsall.bat',
        'tag_location' : os.path.expandvars(r'%appdata%/Microsoft/VisualStudio'),
        'tag_pattern' : r'16.0_(.*)',
        'tag_group_id' : 0
    },
    {
        'name' : 'Visual Studio 2017',
        'reg_root' : winreg.HKEY_LOCAL_MACHINE,
        'reg_subkey' : r'SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7',
        'reg_key' : '15.0',
        'vcvarsall' : r'VC\Auxiliary\Build\vcvarsall.bat'
    },
    {
        'name' : 'Visual Studio 2015',
        'reg_root' : winreg.HKEY_LOCAL_MACHINE,
        'reg_subkey' : r'SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7',
        'reg_key' : '14.0',
        'vcvarsall' : r'vcvarsall.bat'
    }
]

def get_pe_hash(pefilepath):
    '''
    获取 PE 文件的哈希值
    '''
    if not Common.is_file_exists(pefilepath):
        return None
    pe = pefile.PE(pefilepath, fast_load=True)
    return '%X%X' % (pe.FILE_HEADER.TimeDateStamp, pe.OPTIONAL_HEADER.SizeOfImage)

def get_pdb_hash(pdbfilepath):
    '''
    获取 PDB 文件的哈希值
    '''
    if not Common.is_file_exists(pdbfilepath):
        return None
    p = pdbparse.parse(pdbfilepath, fast_load = True)
    pdb = p.streams[pdbparse.PDB_STREAM_PDB]
    pdb.load()
    guidstr = (u'%08x%04x%04x%s%x' % (pdb.GUID.Data1, pdb.GUID.Data2, pdb.GUID.Data3, binascii.hexlify(
        pdb.GUID.Data4).decode('ascii'), pdb.Age)).upper()
    return guidstr

def deploy_symbols(dir_path, symstore_path):
    '''
    扫描某个目录中的全部 exe 和 pdb 文件
    将它们放到符号仓库中
    '''
    for dirpath, _dirnames, filenames in os.walk(dir_path):
        for filename in filenames:
            _base_name, extension_name = os.path.splitext(filename.lower())

            if extension_name not in ['.exe', '.dll', '.pdb']:
                continue

            fullpath = os.path.normpath('%s/%s' % (dirpath, filename))
            deploy_single_symbols(fullpath, symstore_path)

def deploy_single_symbols(filepath, symstore_path):
    '''
    将给定的 exe 和 pdb 等 PE 文件放到符号仓库中
    '''
    filename = os.path.basename(filepath)
    _base_name, extension_name = os.path.splitext(filename.lower())

    if extension_name not in ['.exe', '.dll', '.pdb']:
        return
    
    if extension_name == '.exe':
        filehash = get_pe_hash(filepath)
    else:
        filehash = get_pdb_hash(filepath)
    
    target_filepath = "{symstore_path}/{filebasename}/{hash}/{filename}".format(
        symstore_path = symstore_path,
        filebasename = filename, hash = filehash, filename = filename
    )
    os.makedirs(os.path.dirname(target_filepath), exist_ok=True)
    shutil.copyfile(filepath, target_filepath)

def get_reg_value(vs):
    '''
    根据 vs_configure 中的某个配置项, 获取 Visual Studio 的安装目录
    '''
    reg_subkey = vs['reg_subkey']
    if '{tag}' in vs['reg_subkey']:
        dirs = os.listdir(vs['tag_location'])
        for dirname in dirs:
            matches = re.findall(vs['tag_pattern'], dirname)
            if matches is None or len(matches) != 1:
                continue
            reg_subkey = vs['reg_subkey'].format(tag = matches[vs['tag_group_id']])
            break
    return Common.read_regstr(vs['reg_root'], reg_subkey, vs['reg_key'])

def detect_vs():
    '''
    检测是否安装了符合要求的 Visual Studio 版本
    若安装了任何一个则返回 True 以及版本名称, 若一个都没安装则返回 False 以及 None
    '''
    for vs in vs_configure:
        path = get_reg_value(vs)
        if path is not None:
            return True, vs['name']
    return False, None

def get_vcvarsall_path():
    '''
    读取第一个有效 Visual Studio 的 vcvarsall.bat 所在路径
    如果检测不到则返回 None
    '''
    for vs in vs_configure:
        path = get_reg_value(vs)
        vcvarsall_path = None
        if Common.is_dir_exists(path):
            vcvarsall_path = os.path.join(path, vs['vcvarsall'])
        if Common.is_file_exists(vcvarsall_path):
            return vcvarsall_path
    return None

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

def get_pandas_ver():
    '''
    读取当前 Pandas 在 src/config/pandas.hpp 定义的版本号
    若读取不到版本号则返回 None
    '''
    filepath = os.path.abspath(slndir('src/config/pandas.hpp'))
    if not Common.is_file_exists(filepath):
        return None
    matchgroup = match_file_regex(filepath, r'#define Pandas_Version "(.*)"')
    return matchgroup[0] if matchgroup is not None else None

def get_compile_result():
    '''
    读取编译的结果, 返回四个值分别是:
    读取成功与否, 成功项目数, 失败项目数, 跳过项目数
    '''
    filepath = os.path.abspath(slndir(compile_logfile))
    if not Common.is_file_exists(filepath):
        return False, -1, -1, -1
    
    pattern_list = [
        r'生成: 成功 (\d*) 个，失败 (\d*) 个，跳过 (\d*) 个'
    ]

    for pattern in pattern_list:
        matchgroup = match_file_regex(filepath, pattern)
        if matchgroup is not None and len(matchgroup) == 3:
            return True, matchgroup[0], matchgroup[1], matchgroup[2]

    return False, -1, -1, -1

def has_changelog(ver):
    '''
    判断一个版本的信息是否存在于 Changelog.txt 中
    存在返回 True 不在则返回 False
    '''
    filepath = os.path.abspath(slndir('Changelog.txt'))
    if not Common.is_file_exists(filepath):
        return None
    matchgroup = match_file_regex(filepath, ver)
    return True if matchgroup is not None else False

def save_symbols(cache_subdir):
    '''
    编译完成后进行符号文件的保存工作
    '''
    symbols_cache_dir = os.path.join(slndir(project_cachedir), cache_subdir)
    shutil.rmtree(symbols_cache_dir, ignore_errors=True)
    os.makedirs(symbols_cache_dir, exist_ok=True)

    # 复制需要保存符号的相关文件到特定目录
    compiled_product = [
        'login-server.exe', 'login-server.pdb',
        'char-server.exe', 'char-server.pdb',
        'map-server.exe', 'map-server.pdb',
        'csv2yaml.exe', 'csv2yaml.pdb',
        'mapcache.exe', 'mapcache.pdb'
    ]

    for filepath in compiled_product:
        shutil.copyfile(slndir(filepath), '%s/%s' % (symbols_cache_dir, filepath))
    
    # 将符号文件保存到本地某个目录
    deploy_symbols(symbols_cache_dir, "%s/symbols/win" % project_symstoredir)

    # 使符号仓库将新增的文件设置为待提交
    repo = git.Repo(project_symstoredir)
    repo.git.add('.')

    return True

def slndir(path):
    '''
    基于工程文件根目录进行相对路径的转换
    '''
    return os.path.abspath('%s/%s' % (project_slndir, path))

def clean_environment():
    '''
    清理仓库的工作环境
    '''
    Common.glob_delete(slndir('login-server*'))
    Common.glob_delete(slndir('char-server*'))
    Common.glob_delete(slndir('map-server*'))
    Common.glob_delete(slndir('csv2yaml*'))
    Common.glob_delete(slndir('mapcache*'))

    Common.glob_delete(slndir('logserv.bat'))
    Common.glob_delete(slndir('charserv.bat'))
    Common.glob_delete(slndir('mapserv.bat'))
    Common.glob_delete(slndir('serv.bat'))
    Common.glob_delete(slndir('runserver.bat'))

    Common.glob_delete(slndir('dbghelp.dll'))
    Common.glob_delete(slndir('libmysql.dll'))
    Common.glob_delete(slndir('pcre8.dll'))
    Common.glob_delete(slndir('zlib.dll'))

    shutil.rmtree(slndir(project_cachedir), ignore_errors=True)

def update_symstore():
    if not Common.is_dir_exists(project_symstoredir):
        print('')
        if not Inputer().requireBool({
            'tips' : '本机尚未克隆最新的符号仓库, 是否立刻进行克隆?',
            'default' : False
        }):
            Message.ShowStatus('您主动放弃了继续操作')
            Common.exit_with_pause(-1)

        git.Repo.clone_from(symbols_giturl, project_symstoredir)
    else:
        repo = git.Repo(project_symstoredir)
        if not repo.is_dirty() and not repo.untracked_files:
            Message.ShowStatus('正在尝试拉取最新的符号数据...')
            try:
                repo.remotes.origin.pull()
            except git.GitError as _err:
                Message.ShowWarning('符号数据更新失败: %s' % _err)
            else:
                Message.ShowStatus('符号数据更新成功.')
        else:
            print('')
            if Inputer().requireBool({
                'tips' : '本地符号仓库的工作区不干净, 是否重置工作区?',
                'default' : False
            }):
                repo.git.reset('--hard')
                repo.git.clean('-xdf')
                Message.ShowStatus('已成功重置本地符号仓库到干净状态.')
            else:
                Message.ShowWarning('本地符号仓库工作区不干净, 放弃更新符号数据..')
            print('')

def compile_sub(define_val, name, version, scheme = 'Release|Win32'):
    '''
    用于执行编译
    '''
    # 获取第一个支持的 Visual Studio 对应的 vcvarsall 路径
    vcvarsall = get_vcvarsall_path()

    # 根据名称确定一下标记
    modetag = ('RE' if name == '复兴后' else 'PRE')

    # 执行编译
    Common.cmd_execute([
        '"%s" x86' % vcvarsall,
        'set CL=%s' % define_val,
        'Devenv rAthena.sln /clean',
        'Devenv rAthena.sln /Rebuild "%s"' % scheme
    ], project_slndir, modetag, slndir(compile_logfile))

    # 编译完成后, 判断编译是否成功
    result, succ, faild, _skip = get_compile_result()
    if not result:
        Message.ShowError('无法获取 %s 的编译结果' % name)
        return False

    # 若成功, 则进行符号文件的存储
    if result and int(succ) > 1 and int(faild) == 0:
        Message.ShowStatus('%s: 全部编译成功.' % modetag)
        Message.ShowStatus('%s: 正在存储符号文件到本地符号仓库...' % modetag)
        if save_symbols(modetag):
            Message.ShowStatus('%s: 符号文件存储完毕...' % modetag)
        return True
    else:
        Message.ShowWarning('%s: 存在编译失败的工程, 暂不保存符号文件...' % modetag)
        return False

def compile_prere(version):
    '''
    编译复兴前版本
    '''
    time.sleep(3)
    print('')
    if not compile_sub('/DPRERE', '复兴前', version):
        Message.ShowError('编译复兴前版本且保存符号文件期间发生了一些错误, 请检查...')
        Common.exit_with_pause(-1)

def compile_renewal(version):
    '''
    编译复兴后版本
    '''
    time.sleep(3)
    print('')
    if not compile_sub('', '复兴后', version):
        Message.ShowError('编译复兴前版本且保存符号文件期间发生了一些错误, 请检查...')
        Common.exit_with_pause(-1)

def main():
    '''
    主入口函数
    '''
    # 显示欢迎信息
    Common.welcome('编译流程辅助脚本')

    # 只能在 Windows 环境运行
    if platform.system() != 'Windows':
        Message.ShowError('很抱歉, 此脚本只能在 Windows 环境上运行')
        Common.exit_with_pause(-1)

    # 判断本机是否安装了支持的 Visual Studio
    detected_vs, vs_name = detect_vs()
    if not detected_vs:
        Message.ShowError('无法检测到合适的 Visual Studio 版本 (2015 或 2017)')
        Common.exit_with_pause(-1)
    else:
        Message.ShowStatus('检测到已安装: %s 应可正常编译' % vs_name)

    print('')

    # 读取当前的 Pandas 主程序版本号
    pandas_ver = get_pandas_ver()
    Message.ShowInfo('当前模拟器的主版本是: %s' % pandas_ver)

    # 判断是否已经写入了对应的更新日志, 若没有则要给予提示再继续
    if (has_changelog(pandas_ver)):
        Message.ShowStatus('已经在更新日志中找到了 %s 的版本信息.' % pandas_ver)
    else:
        Message.ShowWarning('没有在更新日志中找到 %s 版本的信息, 请注意完善!' % pandas_ver)

    # 检查创建并尝试更新符号仓库
    update_symstore()

    # 判断当前 git 工作区是否干净, 若工作区不干净要给予提示
    if git.Repo(project_slndir).is_dirty():
        if not Inputer().requireBool({
            'tips' : '当前模拟器代码仓库的工作区不干净, 要重新编译吗?',
            'default' : False
        }):
            Message.ShowStatus('您主动放弃了继续操作')
            Common.exit_with_pause(-1)
    else:
        Message.ShowStatus('当前模拟器代码仓库的工作区是干净的.')
    Message.ShowStatus('即将开始编译, 编译速度取决于电脑性能, 请耐心...')

    # 清理目前的工作目录, 把一些可明确移除的删掉
    clean_environment()

    # 编译 Pandas 的复兴前版本
    compile_prere(pandas_ver)

    # 将复兴前版本的编译产物重命名一下, 避免编译复兴后版本时被覆盖
    shutil.move(slndir('login-server.exe'), slndir('login-server-pre.exe'))
    shutil.move(slndir('login-server.pdb'), slndir('login-server-pre.pdb'))
    shutil.move(slndir('char-server.exe'), slndir('char-server-pre.exe'))
    shutil.move(slndir('char-server.pdb'), slndir('char-server-pre.pdb'))
    shutil.move(slndir('map-server.exe'), slndir('map-server-pre.exe'))
    shutil.move(slndir('map-server.pdb'), slndir('map-server-pre.pdb'))

    # 编译 Pandas 的复兴后版本
    print('')
    compile_renewal(pandas_ver)

    print('')
    Message.ShowStatus('编译工作已经全部结束.')
    Message.ShowStatus('刚刚编译时产生的符号文件已存储, 记得提交符号文件.\n')

    # 友好退出, 主要是在 Windows 环境里给予暂停
    Common.exit_with_pause(0)

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt as _err:
        pass
