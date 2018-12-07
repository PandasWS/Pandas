# -*- coding: utf-8 -*-

from libs import CommonFunc


class InputController:
    def requireText(self, options):
        print('-' * 70)
        print(options['tips'] + ':')
        result = input(options['prefix'])
        if not result:
            print('[提示] 错误, 请至少输入一个字符. 程序终止')
            print('-' * 70)
            CommonFunc().friendly_exit(-1)
        result = options['prefix'] + result
        if options['upper']:
            result = result.upper()
        print('[提示] 您输入的是: ' + result)
        print('-' * 70)
        print('\n')
        return result

    def requireSelect(self, options):
        print('-' * 70)
        select_name = options['name']
        select_data = options['data']
        print('[提示] 请选择%s, 可选值为 [0~%d], 分别代表:' % (select_name, len(select_data) - 1))
        print('')
        for select in select_data:
            print('[选项] %s' % select['desc'])
        print('')
        user_select = input('[选择] 请选择%s [0~%d]:' % (select_name, len(select_data) - 1))

        if not user_select or not user_select.isnumeric():
            print('[提示] 您输入了无效的地图标记类型, 程序终止')
            print('-' * 70)
            CommonFunc().friendly_exit(-1)

        user_select = int(user_select)

        if user_select < 0 or user_select >= len(select_data):
            print('[提示] 您输入了无效的地图标记类型, 程序终止')
            print('-' * 70)
            CommonFunc().friendly_exit(-1)

        print('')
        print('[提示] 您选择的是: %s' % select_data[user_select]['name'])
        print('-' * 70)
        print('\n')
        return user_select

    def requireBool(self, options):
        print('-' * 70)
        user_input = input(options['tips'] + ' [Y/N]:')
        if user_input.lower() in ['y', 'yes']:
            result = True
        elif user_input.lower() in ['n', 'no']:
            result = False
        else:
            result = options['default']
        print('[提示] 您输入的是: ' + ('Yes 是的' if result else 'No 不是'))
        print('-' * 70)
        print('\n')
        return result
