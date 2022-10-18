# -*- coding: utf-8 -*-

import os
import shutil
from dotenv import load_dotenv
from libs import Common, Message

__all__ = [
    'Load',
    'Get',
    'Set',
    'Exists',
    'In'
]

def Load(path: str = '.secret.env', encoding = 'UTF-8'):
    '''
    加载指定路径的 .env 文件
    '''
    # 若无配置信息则自动复制一份文件出来
    if not Common.is_file_exists(path) and Common.is_file_exists('.secret.env.sample'):
        shutil.copyfile('.secret.env.sample', path)

    # 加载 .env 中的配置信息
    if Common.is_file_exists(path):
        load_dotenv(path, encoding=encoding)
    else:
        raise Exception('Secret file not found: ' + path)

def Set(name: str, value):
    '''
    设置指定名称的环境变量
    '''
    os.environ[name] = value

def Get(name: str, default = None, bDefaultThenSet = False)->str:
    '''
    获取指定名称的环境变量 (通常用于保存密钥)
    '''
    if name in os.environ:
        return os.getenv(name)
    else:
        if bDefaultThenSet:
            Set(name, default)
        return default

def Exists(name)->bool:
    '''
    检查指定名称的环境变量是否存在
    '''
    if name in os.environ:
        return True
    else:
        return False

def In(value, name, split = ',')->bool:
    '''
    检查指定名称的环境变量是否包含指定的值
    '''
    if Exists(name):
        val = Get(name)
        if not val:
            return False
        return value in val.split(split)
    else:
        return False
