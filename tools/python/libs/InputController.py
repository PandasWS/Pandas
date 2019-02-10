# -*- coding: utf-8 -*-

from libs import Common, Message

__all__ = [
	'InputController'
]

class InputController:
    def requireText(self, options):
        print('-' * 70)
        Message.ShowSelect(options['tips'] + ':')
        result = input(options['prefix'])
        if not result:
            Message.ShowError('请至少输入一个字符. 程序终止')
            print('-' * 70)
            Common.exitWithPause(-1)
        result = options['prefix'] + result
        if options['upper']:
            result = result.upper()
        Message.ShowInfo('您输入的是: ' + result)
        print('-' * 70)
        print('\n')
        return result

    def requireSelect(self, options):
        print('-' * 70)
        select_name = options['name']
        select_data = options['data']
        Message.ShowSelect('请选择%s, 可选值为 [0~%d], 分别代表:' % (select_name, len(select_data) - 1))
        print('')
        for select in select_data:
            Message.ShowMenu('%s' % select['desc'])
        print('')
        Message.ShowSelect('请选择%s [0~%d]:' % (select_name, len(select_data) - 1), end = "")
        user_select = input()

        if not user_select or not user_select.isnumeric():
            Message.ShowError('您输入了无效的地图标记类型, 程序终止')
            print('-' * 70)
            Common.exitWithPause(-1)

        user_select = int(user_select)

        if user_select < 0 or user_select >= len(select_data):
            Message.ShowError('您输入了无效的地图标记类型, 程序终止')
            print('-' * 70)
            Common.exitWithPause(-1)

        print('')
        Message.ShowInfo('您选择的是: %s' % select_data[user_select]['name'])
        print('-' * 70)
        print('\n')
        return user_select

    def requireBool(self, options):
        print('-' * 70)
        Message.ShowSelect(options['tips'] + ' [Y/N]:', end = "")
        user_input = input()
        if user_input.lower() in ['y', 'yes']:
            result = True
        elif user_input.lower() in ['n', 'no']:
            result = False
        else:
            result = options['default']
        Message.ShowInfo('您输入的是: ' + ('Yes 是的' if result else 'No 不是'))
        print('-' * 70)
        print('\n')
        return result
