'''
//===== Pandas Python Script ============================== 
//= 符号归档辅助脚本
//===== By: ================================================== 
//= Sola丶小克
//===== Current Version: ===================================== 
//= 1.0
//===== Description: ========================================= 
//= 此脚本用于将当前工作目录的符号文件进行归档存储
//===== Additional Comments: ================================= 
//= 1.0 首个版本. [Sola丶小克]
//============================================================
'''

# -*- coding: utf-8 -*-

import os
import git
import shutil

from libs import Common, Inputer, Message

# 切换工作目录为脚本所在目录
os.chdir(os.path.split(os.path.realpath(__file__))[0])

# 工程文件的主目录相对此脚本文件的位置
project_slndir = '../../'

# 符号仓库工程路径
project_symstoredir = os.path.abspath(project_slndir + '../Symbols')

# 符号文件的 Git 仓库路径
symbols_giturl = 'https://github.com/PandasWS/Symbols.git'

def ensure_store():
    if not Common.is_dir_exists(project_symstoredir):
        if not Inputer().requireBool({
            'tips' : '本机尚未克隆最新的符号仓库, 是否立刻进行克隆?',
            'default' : False
        }):
            return False

        git.Repo.clone_from(symbols_giturl, project_symstoredir)
    return True

def ensure_store_clean():
    if not Common.is_dir_exists(project_symstoredir):
        return False

    repo = git.Repo(project_symstoredir)
    if not repo.is_dirty() and not repo.untracked_files:
        return True

    if Inputer().requireBool({
        'tips' : '本地符号仓库的工作区不干净, 是否重置工作区?',
        'default' : False
    }):
        repo.git.reset('--hard')
        repo.git.clean('-xdf')
        Message.ShowStatus('已成功重置本地符号仓库到干净状态.')
        return True
    return False

def update_store():
    repo = git.Repo(project_symstoredir)
    if repo.is_dirty() or repo.untracked_files:
        return False
    
    Message.ShowStatus('正在尝试拉取最新的符号数据, 请稍候...')
    try:
        repo.remotes.origin.pull()
    except git.GitError as _err:
        Message.ShowWarning('更新失败, 原因: %s' % _err)
        return False
    else:
        return True

def deploy_file(filepath):
    basename = os.path.basename(filepath)
    extname = Common.get_file_ext(basename)

    if extname not in ['.exe', '.dll', '.pdb']:
        return
    
    if extname == '.pdb':
        filehash = Common.get_pdb_hash(filepath)
    elif extname in ['.dll', '.exe']:
        filehash = Common.get_pe_hash(filepath)

    target_filepath = '{symstore}/{filename}/{hash}/{filename}'.format(
        symstore = project_symstoredir, hash = filehash,
        filename = basename.replace('-pre', '')
    )
    os.makedirs(os.path.dirname(target_filepath), exist_ok=True)
    shutil.copyfile(filepath, target_filepath)

def deploy_symbols(sourcedir):
    # 要排除的目录或者文件 (小写)
    exclude_dirs = ['3rdparty', '.vs']
    exclude_files = ['libmysql.dll', 'dbghelp.dll', 'pcre8.dll', 'zlib.dll']

    for dirpath, dirnames, filenames in os.walk(sourcedir):
        dirnames[:] = [d for d in dirnames if d.lower() not in exclude_dirs]
        filenames[:] = [f for f in filenames if f.lower() not in exclude_files]
        for filename in filenames:
            extname = Common.get_file_ext(filename)
            if extname not in ['.exe', '.dll', '.pdb']:
                continue
            fullpath = os.path.join(dirpath, filename)
            deploy_file(fullpath)

def make_commit():
    try:
        repo = git.Repo(project_symstoredir)

        if not repo.is_dirty() and not repo.untracked_files:
            Message.ShowWarning('工作区很干净, 没有任何变化文件, 无需提交.')
            return False

        repo.git.add('.')
        repo.git.commit('-m', '归档 Pandas {version} 版本的符号文件和编译产物, 代码源自 PandasWS/Pandas@{githash} 仓库'.format(
            version = Common.get_pandas_ver(project_slndir, 'v'),
            githash = Common.get_pandas_hash(project_slndir)
        ))
    except git.GitError as _err:
        Message.ShowWarning('提交失败, 原因: %s' % _err)
        return False
    else:
        return True

def main():
    # 显示欢迎信息
    Common.welcome('符号归档辅助脚本')
    print('')
    
    # 检查是否已经完成了编译
    if not Common.is_compiled(project_slndir):
        Message.ShowWarning('检测到打包需要的编译产物不完整, 请重新编译. 程序终止.')
        Common.exit_with_pause(-1)

    # 检查符号仓库是否存在, 不存在则创建
    if not ensure_store():
        Message.ShowWarning('你放弃了拉取符号仓库, 后续工作无法继续. 程序终止.')
        Common.exit_with_pause(-1)
    else:
        Message.ShowStatus('符号仓库已经存在于本地, 开始确认工作区状态...')

    # 若符号文件存在, 则确保工作区干净
    if not ensure_store_clean():
        Message.ShowWarning('为了防止出现意外, 请确保符号仓库的工作区是干净的. 程序终止.')
        Common.exit_with_pause(-1)
    else:
        Message.ShowStatus('很好, 符号仓库的工作区是干净的.')
    
    # 尝试更新本地的符号仓库到最新
    if not update_store():
        Message.ShowWarning('为了防止出现意外, 请确保符号仓库同步到最新. 程序终止.')
        Common.exit_with_pause(-1)
    else:
        Message.ShowStatus('符号仓库已经同步到最新, 开始归档新的符号文件...')

    # 搜索工程目录全部 pdb 文件和 exe 文件, 进行归档
    deploy_symbols(project_slndir)
    
    # 自动进行 git 提交操作
    Message.ShowStatus('归档完毕, 正在提交...')
    if not make_commit():
        Message.ShowWarning('很抱歉, 提交失败! 请确认失败的原因. 程序终止.')
        Common.exit_with_pause(-1)
    else:
        Message.ShowStatus('提交成功, 请手工推送到远程仓库.')
    
    # 友好退出, 主要是在 Windows 环境里给予暂停
    Common.exit_with_pause()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt as _err:
        pass