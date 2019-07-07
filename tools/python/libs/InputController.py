# -*- coding: utf-8 -*-

from libs import Common, Message

__all__ = [
    'Inputer'
]

class Inputer:
    def requireText(self, options):
        print('-' * 70)
        Message.ShowSelect(options['tips'] + ':')
        prefix_val = options['prefix'] if 'prefix' in options else ''
        result = input(prefix_val)
        if not result:
            if 'default' in options and options['default']:
                result = options['default']
            if not result and not ('allow_empty' in options and options['allow_empty']):
                Message.ShowError('请至少输入一个字符. 程序终止')
                print('-' * 70)
                Common.exit_with_pause(-1)
        result = prefix_val + result
        if 'upper' in options and options['upper']:
            result = result.upper()
        if 'lower' in options and options['lower']:
            result = result.lower()
        Message.ShowInfo('您输入的是: ' + result)
        print('-' * 70)
        print('')
        return result

    def requireInt(self, options):
        print('-' * 70)
        Message.ShowSelect(options['tips'] + ':')
        result = input(options['prefix']) if 'prefix' in options else input()
        if not result:
            if not ('allow_empty' in options and options['allow_empty']):
                Message.ShowError('请至少输入一个数字. 程序终止')
                print('-' * 70)
                Common.exit_with_pause(-1)
            result = '0'
        if not result.isdigit():
            Message.ShowError('请输入一个数字而不是字符串. 程序终止')
            print('-' * 70)
            Common.exit_with_pause(-1)
        Message.ShowInfo('您输入的是: ' + result)
        print('-' * 70)
        print('')
        return int(result)

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
            Common.exit_with_pause(-1)

        user_select = int(user_select)

        if user_select < 0 or user_select >= len(select_data):
            Message.ShowError('您输入了无效的地图标记类型, 程序终止')
            print('-' * 70)
            Common.exit_with_pause(-1)

        print('')
        Message.ShowInfo('您选择的是: %s' % select_data[user_select]['name'])
        print('-' * 70)
        print('')
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
        print('')
        return result
