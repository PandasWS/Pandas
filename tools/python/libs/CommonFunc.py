# -*- coding: utf-8 -*-

import os
import platform


class CommonFunc():
    def friendly_exit(self, exitcode = 0):
        if platform.system() == 'Windows':
            os.system('pause')
        exit(exitcode)
