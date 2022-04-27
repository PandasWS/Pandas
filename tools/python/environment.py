# -*- coding: utf-8 -*-

import os
import sys
import inspect
import platform
import subprocess


__current_path__ = os.path.split(os.path.realpath(__file__))[0]
__current_path__ = os.path.abspath(__current_path__) + os.path.sep

def call(cmdlines, cwd=None):
    process = subprocess.Popen(cmdlines, cwd=cwd, shell=True)
    process.wait()
    return process.returncode

def in_virtualenvs():
    return sys.executable.lower().find('virtualenvs') >= 0

def pipenv_ready():
    try:
        import pipenv
    except ImportError:
        return False
    return True

def pipenv_install():
    call('pip install pipenv --index-url https://mirrors.aliyun.com/pypi/simple/', cwd=__current_path__)

def pipenv_cfg_exist():
    return os.path.exists(__current_path__ + 'Pipfile') and os.path.exists(__current_path__ + 'Pipfile.lock')

def initialize():
    # 若不是 Windows 系统则什么都不做
    # 使用非 Windows 操作系统的用户我们假定他什么都懂..
    if platform.system() != 'Windows':
        return

    # 判断是否当前已经运行在 pipenv 的虚拟环境中,
    # 若是, 那么直接返回即可, 无需我们做其他额外的操作了
    if in_virtualenvs():
        return
    
    # 检查是否已经安装了 pipenv 这个库, 没有就安装一下
    if not pipenv_ready():
        pipenv_install()
    
    # 再判断一下是否安装成功了, 失败则报错且继续
    if not pipenv_ready():
        print('尝试自动安装 pipenv 失败, 请检查环境.')
        return

    # 若不是, 则根据 Pipfile.lock 的配置去安装虚拟环境
    elif pipenv_cfg_exist():
        call('pipenv install', cwd=__current_path__)

    # 完成了 install 之后, 在新的 pipenv 会话中启动当前脚本
    call('start cmd /K "{cmdline}"'.format(
        cmdline = ' && '.join([
            'pipenv run "%s"' % inspect.stack()[-1][1]
        ])
    ), cwd=__current_path__)
    
    # 老的脚本会话可以直接终止了
    exit(0)

if __name__ == '__main__':
    print('')
    print(r"                                                                              ")
    print(r"                            Pandas Dev Team Presents                          ")
    print(r"                     ____                    _                                ")
    print(r"                    |  _ \  __ _  _ __    __| |  __ _  ___                    ")
    print(r"                    | |_) |/ _` || '_ \  / _` | / _` |/ __|                   ")
    print(r"                    |  __/| (_| || | | || (_| || (_| |\__ \                   ")
    print(r"                    |_|    \__,_||_| |_| \__,_| \__,_||___/                   ")
    print(r"                                                                              ")
    print(r"                               https://pandas.ws/                             ")
    print(r"                                                                              ")
    print(r"               Pandas is only for learning and research purposes.             ")
    print(r"                      Please don't use it for commercial.                     ")
    print(r"                                                                              ")
    print('')
    print('[信息] 该文件单独运行时没任何作用, 但请不要删除它.')
    print('[信息] 它负责在 Windows 环境下直接双击 py 脚本时, 能够自动准备运行环境.')
    if platform.system() == 'Windows':
        print('')
        os.system('pause')
    exit(0)
