# -*- coding: utf-8 -*-

from colorama import init
from colorama import Fore, Back, Style

init()

def ShowInfo(text):
    print(Fore.WHITE + '[信息] ' + Fore.RESET + text)

def ShowStatus(text):
    print(Fore.GREEN + '[状态] ' + Fore.RESET + text)
