# -*- coding: utf-8 -*-

import os
import platform


def exitWithPause(exitcode = 0):
    if platform.system() == 'Windows':
        os.system('pause')
    exit(exitcode)
