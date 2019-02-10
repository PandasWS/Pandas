# -*- coding: utf-8 -*-

from colorama import init
from colorama import Fore

__all__ = [
	'ShowInfo',
	'ShowStatus',
	'ShowNotice',
	'ShowWarning',
	'ShowError',
	'ShowSelect',
	'ShowMenu'
]

init()

def ShowInfo(text, **kwargs):
    print(Fore.WHITE + '[信息] ' + Fore.RESET + text, **kwargs)

def ShowStatus(text, **kwargs):
    print(Fore.GREEN + '[状态] ' + Fore.RESET + text, **kwargs)

def ShowNotice(text, **kwargs):
    print(Fore.WHITE + '[注意] ' + Fore.RESET + text, **kwargs)

def ShowWarning(text, **kwargs):
    print(Fore.YELLOW + '[警告] ' + Fore.RESET + text, **kwargs)

def ShowError(text, **kwargs):
    print(Fore.RED + '[错误] ' + Fore.RESET + text, **kwargs)

def ShowSelect(text, **kwargs):
    print(Fore.GREEN + '[选择] ' + Fore.RESET + text, **kwargs)

def ShowMenu(text, **kwargs):
    print(Fore.WHITE + '[选项] ' + Fore.RESET + text, **kwargs)
