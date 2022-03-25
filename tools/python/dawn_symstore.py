'''
//===== Pandas Python Script =================================
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

import environment
environment.initialize()

import os
import shutil
import platform
import oss2
import glob

from dotenv import load_dotenv
from libs import Common, Message

# 切换工作目录为脚本所在目录
os.chdir(os.path.split(os.path.realpath(__file__))[0])

# 工程文件的主目录相对此脚本文件的位置
project_slndir = '../../'

# 符号仓库工程路径 (在 main 函数中赋值)
project_symstoredir = ''

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
    exclude_files = [
        'libmysql.dll', 'dbghelp.dll', 'pcre8.dll', 'zlib.dll',
        'vmprotectsdk32.dll', 'vmprotectsdk64.dll'
    ]

    for dirpath, dirnames, filenames in os.walk(sourcedir):
        dirnames[:] = [d for d in dirnames if d.lower() not in exclude_dirs]
        filenames[:] = [f for f in filenames if f.lower() not in exclude_files]
        for filename in filenames:
            extname = Common.get_file_ext(filename)
            if extname not in ['.exe', '.dll', '.pdb']:
                continue
            fullpath = os.path.join(dirpath, filename)
            deploy_file(fullpath)

def upload_symbols(symstoredir, only_upload_new = False):
    # 初始化阿里云 OSS
    auth = oss2.Auth(os.getenv('OSS_SYMBOLS_FULL_ACCESS_KEY_ID'), os.getenv('OSS_SYMBOLS_FULL_ACCESS_KEY_SECRET'))
    endpoint = os.getenv('OSS_SYMBOLS_ENDPOINT')
    bucket_name = os.getenv('OSS_SYMBOLS_BUCKET_NAME')
    bucket = oss2.Bucket(auth, endpoint, bucket_name)

    if (symstoredir.endswith('/')):
        symstoredir = symstoredir[:-1]

    # 列举本地指定目录下的全部文件    
    local_files = glob.glob('{symstoredir}/**/*'.format(symstoredir = symstoredir), recursive=True)

    # 只保留文件, 剔除掉目录
    local_files = [f for f in local_files if os.path.isfile(f)]

    # 移除需要排除的目录下的文件
    exclude_dir = ['.git']
    exclude_file = ['.DS_Store']
    local_files = [f for f in local_files if not any(os.path.sep + d + os.path.sep in f.lower() for d in exclude_dir)]
    local_files = [f for f in local_files if not any(f.lower().endswith(e) for e in exclude_file)]

    # 准备必要的列表变量用来存储东西
    all_remote_files = []
    final_need_upload = []

    # 如果要求只上传本地才有的新文件, 则需要先列举出所有的远程文件
    if only_upload_new:
        list_result = bucket.list_objects(prefix = '')
        while list_result:
            remote_files = [x.key for x in list_result.object_list]
            all_remote_files.extend(remote_files)
            if list_result.is_truncated:
                list_result = bucket.list_objects(prefix = '', marker = list_result.next_marker)
            else:
                break
        
        # 此时 all_remote_files 中已经包含了所有的远程文件
        # 移除其中以 '/' 结尾的信息, 因为他们是目录
        all_remote_files = [x for x in all_remote_files if not x.endswith('/')]

        # 将 all_remote_files 中的字符串转换成小写
        all_remote_files = [x.lower() for x in all_remote_files]

        # 从 local_files 中排除 all_remote_files 存在的记录
        for x in local_files:
            key = os.path.relpath(x, symstoredir).lower().replace('\\', '/')
            if key in all_remote_files:
                Message.ShowInfo('已经跳过: ' + key)
                continue
            final_need_upload.append(x)
    else:
        final_need_upload = local_files

    # TODO: 将文件压缩成 cab 以便节省体积

    # 执行文件上传工作
    for filepath in final_need_upload:
        key = os.path.relpath(x, symstoredir).lower().replace('\\', '/')
        Message.ShowInfo('正在上传: {filepath}'.format(filepath = key))
        try:
            bucket.put_object_from_file(key, filepath)
        except Exception as e:
            Message.ShowError(str(e))
            pass

def main():
    # 加载 .env 中的配置信息
    load_dotenv(dotenv_path='.config.env', encoding='UTF-8')
    
    # 若无配置信息则自动复制一份文件出来
    if not Common.is_file_exists('.config.env'):
        shutil.copyfile('.config.env.sample', '.config.env')

    # 显示欢迎信息
    Common.welcome('符号归档辅助脚本')
    print('')
    
    # 由于 pdbparse 只能在 Windows 环境下安装, 此处进行限制
    if platform.system() != 'Windows':
        Message.ShowWarning('该脚本只能在 Windows 环境下运行, 程序终止.')
        Common.exit_with_pause(-1)
    
    # 若环境变量为空则设置个默认值
    if not os.getenv('DEFINE_PROJECT_NAME'):
        os.environ["DEFINE_PROJECT_NAME"] = "Pandas"

    if not os.getenv('DEFINE_COMPILE_MODE'):
        os.environ["DEFINE_COMPILE_MODE"] = "re,pre"
    
    # 符号仓库工程路径
    global project_symstoredir
    project_symstoredir = os.path.abspath(project_slndir + '../Symbols')
    Message.ShowInfo('当前输出的项目名称为: %s' % os.getenv('DEFINE_PROJECT_NAME'))

    # 检查是否已经完成了编译
    if 're' in os.getenv('DEFINE_COMPILE_MODE').split(','):
        if not Common.is_compiled(project_slndir, checkmodel='re'):
            Message.ShowWarning('检测到打包需要的编译产物不完整, 请重新编译. 程序终止.')
            Common.exit_with_pause(-1)

    if 'pre' in os.getenv('DEFINE_COMPILE_MODE').split(','):
        if not Common.is_compiled(project_slndir, checkmodel='pre'):
            Message.ShowWarning('检测到打包需要的编译产物不完整, 请重新编译. 程序终止.')
            Common.exit_with_pause(-1)

    # 搜索工程目录全部 pdb 文件和 exe 文件, 进行归档
    deploy_symbols(project_slndir)
    Message.ShowStatus('符号文件已经归档完毕, 正在同步到符号服务器...')
    
    # 将符号文件归档到符号服务器
    upload_symbols(project_symstoredir, True)

    Message.ShowStatus('符号文件已经同步完毕.')

    # 友好退出, 主要是在 Windows 环境里给予暂停
    Common.exit_with_pause()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt as _err:
        pass
