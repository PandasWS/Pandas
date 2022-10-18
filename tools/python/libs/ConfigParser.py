# -*- coding: utf-8 -*-

import os
import re
import yaml

from libs import Common, SecretParser

__all__ = [
    'Parser'
]

# 读取 .secret.env 文件中的环境变量
SecretParser.Load()

class ConfigDict(dict):
    '''
    可以使用 . 进行访问的配置字典
    '''
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.__dict__ = self

class Parser:
    __config_data = ConfigDict()
    __commercial = False

    def __init__(self, commercial = False):
        self.__commercial = commercial
        self.load()

    def load(self):
        config_path = os.path.abspath(os.path.dirname(__file__) + '/../config.yml')
        config_encoding = Common.get_file_encoding(config_path)

        with open(config_path, 'rt', encoding=config_encoding) as f:
            self.__config_data = ConfigDict(yaml.safe_load(f))

        return self.__config_data

    def raw_data(self):
        return self.__config_data

    def get_section(self, section_name):
        if section_name in self.__config_data:
            return self.__config_data[section_name]
        else:
            return None

    def get(self, key, section = None, unset_then_none = True):
        value = None
        
        parent_node = None
        if section is None:
            parent_node = self.get_section('default')
        else:
            parent_node = self.get_section(section)

        # 从配置文件中读取默认值
        if key and key in parent_node:
            value = parent_node[key]
        
        # 根据不同版本来读取对应的值进行覆盖
        if self.__commercial:
            parent_node = self.get_section('commercial')
        else:
            parent_node = self.get_section('community')

        if parent_node and key in parent_node:
            value = parent_node[key]

        # 检查是否存在强制覆盖配置
        if self.get_section('override'):
            if key in self.get_section('override'):
                value = self.get_section('override')[key]
    
        # 检查是否需要为字符串执行环境变量替换
        if (value and isinstance(value, str)):
            value = self.__replace_secret(value, unset_then_none)
        return value

    def __replace_secret(self, value, unset_then_none):
        if not isinstance(value, str):
            return value
        
        matched = re.match(r"\$SECRET{'(.*?)'}", value, re.IGNORECASE)
        if not matched:
            return value

        secret_name = matched.group(1)
        if SecretParser.Exists(secret_name):
            return SecretParser.Get(secret_name)
        elif unset_then_none:
            return None
        return value

    def get_protect(self, key, unset_then_none = True):
        return self.get(key, 'protect', unset_then_none)
    
    def get_crashrpt(self, key, unset_then_none = True):
        return self.get(key, 'crashrpt', unset_then_none)

    def get_symbols(self, key, unset_then_none = True):
        return self.get(key, 'symbols', unset_then_none)

    def get_publish(self, key, unset_then_none = True):
        return self.get(key, 'publish', unset_then_none)
