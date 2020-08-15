'''
//===== Pandas Python Script =================================
//= 编译流程辅助脚本
//===== By: ================================================== 
//= Sola丶小克
//===== Current Version: ===================================== 
//= 1.0
//===== Description: ========================================= 
//= 此脚本用于编译复兴前和复兴后 Release 版本的 Pandas 模拟器
//= 符号文件的储存和上传工作, 将由其他脚本来实现
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

import git

from dotenv import load_dotenv
from libs import Common, Inputer, Message

# 切换工作目录为脚本所在目录
os.chdir(os.path.split(os.path.realpath(__file__))[0])

# 工程文件的主目录相对此脚本文件的位置
project_slndir = '../../'

# 此脚本所需要的缓存目录 (缓存符号文件, 编译输出日志)
project_cachedir = 'tools/python/cache/'

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
        matchgroup = Common.match_file_regex(filepath, pattern)
        if matchgroup is not None and len(matchgroup) == 3:
            return True, matchgroup[0], matchgroup[1], matchgroup[2]

    return False, -1, -1, -1

def has_changelog(ver):
    '''
    判断一个版本的信息是否存在于 Changelog.txt 中
    存在返回 True 不在则返回 False
    '''
    filepath = os.path.abspath(slndir('CHANGELOG.md'))
    if not Common.is_file_exists(filepath):
        return None
    matchgroup = Common.match_file_regex(filepath, ver)
    return True if matchgroup is not None else False

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
        return True
    else:
        Message.ShowWarning('%s: 存在编译失败的工程, 请进行检查...' % modetag)
        return False

def define_builder(options):
    '''
    提供给定的一组宏定义数组, 用于生成正确的命令行
    '''
    value = ''
    for key in options:
        value = value + '/D' + str(key)
        if options[key]:
            value = value + '#' + str(options[key])
        value = value + ' '
    return value

def compile_prere(version):
    '''
    编译复兴前版本
    '''
    time.sleep(3)
    print('')
    
    define_options = {
        "PRERE": ""
    }
    
    if os.getenv("DEFINE_CRASHRPT_APPID"):
        define_options["CRASHRPT_APPID"] = '\\"%s\\"' % os.getenv("DEFINE_CRASHRPT_APPID")
    if os.getenv("DEFINE_CRASHRPT_PUBLICKEY"):
        define_options["CRASHRPT_PUBLICKEY"] = '\\"%s\\"' % os.getenv("DEFINE_CRASHRPT_PUBLICKEY")
        
    define_options["GIT_BRANCH"] = '\\"%s\\"' % Common.get_pandas_branch(project_slndir)
    define_options["GIT_HASH"] = '\\"%s\\"' % Common.get_pandas_hash(project_slndir)

    define_values = define_builder(define_options)
    
    if not compile_sub(define_values, '复兴前', version):
        Message.ShowError('编译复兴前版本时发生了一些错误, 请检查...')
        Common.exit_with_pause(-1)

    # 将复兴前版本的编译产物重命名一下, 避免编译复兴后版本时被覆盖
	# 因 ab7a827 的修改每次清理工程时, 也会同时清理复兴前的编译产物, 所以这里需要临时重命名
    shutil.move(slndir('login-server.exe'), slndir('login-server-pre-t.exe'))
    shutil.move(slndir('login-server.pdb'), slndir('login-server-pre-t.pdb'))
    shutil.move(slndir('char-server.exe'), slndir('char-server-pre-t.exe'))
    shutil.move(slndir('char-server.pdb'), slndir('char-server-pre-t.pdb'))
    shutil.move(slndir('map-server.exe'), slndir('map-server-pre-t.exe'))
    shutil.move(slndir('map-server.pdb'), slndir('map-server-pre-t.pdb'))
    
    print('')

def compile_renewal(version):
    '''
    编译复兴后版本
    '''
    time.sleep(3)
    print('')
    
    define_options = {}

    if os.getenv("DEFINE_CRASHRPT_APPID"):
        define_options["CRASHRPT_APPID"] = '\\"%s\\"' % os.getenv("DEFINE_CRASHRPT_APPID")
    if os.getenv("DEFINE_CRASHRPT_PUBLICKEY"):
        define_options["CRASHRPT_PUBLICKEY"] = '\\"%s\\"' % os.getenv("DEFINE_CRASHRPT_PUBLICKEY")

    define_options["GIT_BRANCH"] = '\\"%s\\"' % Common.get_pandas_branch(project_slndir)
    define_options["GIT_HASH"] = '\\"%s\\"' % Common.get_pandas_hash(project_slndir)

    define_values = define_builder(define_options)
    
    if not compile_sub(define_values, '复兴后', version):
        Message.ShowError('编译复兴前版本时发生了一些错误, 请检查...')
        Common.exit_with_pause(-1)
    
    # 将之前 compile_prere 中临时重命名的复兴前产物全部改回正常的文件名
    shutil.move(slndir('login-server-pre-t.exe'), slndir('login-server-pre.exe'))
    shutil.move(slndir('login-server-pre-t.pdb'), slndir('login-server-pre.pdb'))
    shutil.move(slndir('char-server-pre-t.exe'), slndir('char-server-pre.exe'))
    shutil.move(slndir('char-server-pre-t.pdb'), slndir('char-server-pre.pdb'))
    shutil.move(slndir('map-server-pre-t.exe'), slndir('map-server-pre.exe'))
    shutil.move(slndir('map-server-pre-t.pdb'), slndir('map-server-pre.pdb'))
    
    print('')

def main():
    '''
    主入口函数
    '''
    # 加载 .env 中的配置信息
    load_dotenv(dotenv_path='pyhelp.conf', encoding='UTF-8-SIG')
    
    # 若无配置信息则自动复制一份文件出来
    if not Common.is_file_exists('pyhelp.conf'):
        shutil.copyfile('pyhelp.conf.sample', 'pyhelp.conf')
    
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
    pandas_ver = Common.get_pandas_ver(os.path.abspath(project_slndir))
    Message.ShowInfo('当前模拟器的主版本是 %s' % pandas_ver)

    # 判断是否已经写入了对应的更新日志, 若没有则要给予提示再继续
    if (has_changelog(pandas_ver)):
        Message.ShowStatus('已经在更新日志中找到了 %s 的版本信息.' % pandas_ver)
    else:
        Message.ShowWarning('没有在更新日志中找到 %s 版本的信息, 请注意完善!' % pandas_ver)

    # 判断当前 git 工作区是否干净, 若工作区不干净要给予提示
    if git.Repo(project_slndir).is_dirty():
        if not Inputer().requireBool({
            'tips' : '当前模拟器代码仓库的工作区不干净, 要继续编译吗?',
            'default' : False
        }):
            Message.ShowStatus('您主动放弃了继续操作')
            Common.exit_with_pause(-1)
    else:
        Message.ShowStatus('当前模拟器代码仓库的工作区是干净的.')

    # 检查 Crashrpt 使用的信息是否都设置好了, 若没有且企图编译正式版, 则给与提示
    if Common.is_pandas_release(os.path.abspath(project_slndir)):
        if not os.getenv("DEFINE_CRASHRPT_APPID"):
            Message.ShowWarning('当前并未设置 AppID, 且企图编译正式版.')
            Common.exit_with_pause(-1)
        if Common.md5(os.getenv("DEFINE_CRASHRPT_APPID")) != '952648de2d8f063a07331ae3827bc406':
            Message.ShowWarning('当前已设置了 AppID, 但并非正式版使用的 AppID.')
            Common.exit_with_pause(-1)
        if not os.getenv("DEFINE_CRASHRPT_PUBLICKEY"):
            Message.ShowWarning('当前并未设置 PublicKey, 且企图编译正式版.')
            Common.exit_with_pause(-1)
    
    Message.ShowStatus('即将开始编译, 编译速度取决于电脑性能, 请耐心...')

    # 清理目前的工作目录, 把一些可明确移除的删掉
    clean_environment()

    # 编译 Pandas 的复兴前版本
    compile_prere(pandas_ver)

    # 编译 Pandas 的复兴后版本
    compile_renewal(pandas_ver)

    Message.ShowStatus('编译工作已经全部结束, 请归档符号并执行打包流程.')

    # 友好退出, 主要是在 Windows 环境里给予暂停
    Common.exit_with_pause()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt as _err:
        pass
