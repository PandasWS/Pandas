'''
//===== Pandas Python Script =================================
//= 打包流程辅助脚本
//===== By: ================================================== 
//= Sola丶小克
//===== Current Version: ===================================== 
//= 1.0
//===== Description: ========================================= 
//= 此脚本用于生成复兴前后的打包源目录, 并打包成 zip 文件
//===== Additional Comments: ================================= 
//= 1.0 首个版本. [Sola丶小克]
//============================================================
'''

# -*- coding: utf-8 -*-

import environment
environment.initialize()

import os
import git
import pyzipper
import glob
import shutil
import platform
import imple_translate as trans

from dotenv import load_dotenv
from libs import Common, Message, ConfigParser

# 切换工作目录为脚本所在目录
os.chdir(os.path.split(os.path.realpath(__file__))[0])

# 工程文件的主目录相对此脚本文件的位置
project_slndir = '../../'
slndir_path = os.path.abspath(project_slndir)

# 使程序加载 config.yml 中的相关数据
parser = ConfigParser.Parser(Common.is_commercial_ver(slndir_path))

# Git 仓库对象
repo = None
try:
    repo = git.Repo(project_slndir)
except Exception as _err:
    pass

# 判断当前是否为专业版
is_commercial = Common.is_commercial_ver(slndir_path)

# 打包后的输出目录路径
project_publishdir = parser.get_publish('publish_path')
project_publishdir = '../Release' if len(project_publishdir) == 0 else project_publishdir
project_publishdir = os.path.abspath(project_slndir + project_publishdir) if project_publishdir.startswith('..') else project_publishdir

def export():
    '''
    将当前工程目录的全部内容导出成一个 zip 文件
    '''
    export_path = os.path.abspath(project_slndir + 'pandas_export.zip')
    repo.git.archive('HEAD', ':(exclude)3rdparty', '--format=zip', '-o', export_path)
    return export_path if os.path.exists(export_path) else None

def zip_unpack(zipfilename, targetdir):
    '''
    将一个 zip 文件解压缩到指定的目录
    '''
    try:
        zip = pyzipper.ZipFile(zipfilename, 'r')
        zip.extractall(targetdir)
        zip.close()
    except Exception as _err:
        return False
    else:
        return True

def zip_pack(sourcedir, zipfilename, password = None):
    '''
    将一个指定的目录打包成指定的路径 zip 文件
    '''
    try:
        basename = os.path.basename(os.path.normpath(sourcedir))
        z = None

        if password and len(password) > 0:
            z = pyzipper.AESZipFile(
                zipfilename, 'w', compression=pyzipper.ZIP_DEFLATED,
                encryption=pyzipper.WZ_AES
            )
            z.pwd=password.encode('utf-8')
        else:
            z = pyzipper.ZipFile(zipfilename, 'w', compression=pyzipper.ZIP_DEFLATED)

        for dirpath, _dirnames, filenames in os.walk(sourcedir):
            fpath = dirpath.replace(sourcedir, '')
            fpath = fpath and fpath + os.sep or ''
            for filename in filenames:
                inzippath = basename + os.path.sep + fpath + filename
                z.write(os.path.join(dirpath, filename), inzippath)
        z.close()
    except Exception as _err:
        return False
    else:
        return True

def rmdir(dirpath, dir_exclude = [], file_exclude = [], exclude_deep = True):
    '''
    移除指定的目录, 包括子目录和子目录中的文件
    可以通过设置 dir_exclude 排除要删除的目录名称 (小写)
    可以通过设置 file_exclude 排除要删除的文件名称 (小写)
    可以通过设置 exclude_deep 来决定排除名单是否影响子目录的删除策略
    '''
    dirpath = os.path.abspath(dirpath)
    if not os.path.exists(dirpath): return
    if not os.path.isdir(dirpath): return

    for i in glob.glob(os.path.join(dirpath, "*")) + glob.glob(os.path.join(dirpath, ".*")):
        if os.path.isdir(i) and os.path.basename(i).lower() not in dir_exclude:
            if exclude_deep:
                rmdir(i, dir_exclude, file_exclude)
            else:
                rmdir(i)
        elif os.path.isfile(i) and os.path.basename(i).lower() not in file_exclude:
            os.remove(i)
    if not os.listdir(dirpath): os.rmdir(dirpath)

def enum_files(curr_dir = '.', ext = '*.md'):
    '''
    枚举指定目录中特定后缀的文件 (不枚举子目录)
    '''
    for i in glob.glob(os.path.join(curr_dir, ext)):
        if os.path.isdir(i): continue
        yield i

def remove_files(dirpath, ext):
    '''
    删除指定目录中符合特定文件名通配符的文件 (不枚举子目录)
    '''
    dirpath = os.path.abspath(dirpath)
    for filename in enum_files(dirpath, ext):
        os.remove(filename)

def copyfile(srcfile, dstfile):
    '''
    复制某个文件到另外一个位置
    '''
    srcfile = os.path.abspath(srcfile)
    dstfile = os.path.abspath(dstfile)
    shutil.copyfile(srcfile, dstfile)

def copyfiles(srcdir, srcext, targetdir):
    '''
    将指定目录中符合特定文件名通配符的文件, 复制到另外一个目录中
    '''
    srcdir = os.path.abspath(srcdir)
    targetdir = os.path.abspath(targetdir)
    for filename in enum_files(srcdir, srcext):
        basename = os.path.basename(filename)
        targetname = os.path.join(targetdir, basename)
        shutil.copyfile(filename, targetname)

def remove_file(dirpath, filename):
    '''
    删除某个目录中指定的文件
    '''
    filename = os.path.abspath(dirpath + '/' + filename)
    if os.path.exists(filename):
        os.remove(filename)

def recursive_overwrite(src, dest, ignore=None):
    '''
    将来源目录中的文件覆盖到目标目标上
    '''
    if os.path.isdir(src):
        if not os.path.isdir(dest):
            os.makedirs(dest)
        files = os.listdir(src)
        ignored = set() if ignore is None else ignore(src, files)

        for f in files:
            if f not in ignored:
                recursive_overwrite(os.path.join(src, f), os.path.join(dest, f), ignore)
    else:
        shutil.copyfile(src, dest)

def arrange_common(packagedir):
    '''
    复兴前和复兴后都能够通用的整理规则
    无论是对哪个版本进行整理, 都需要调用一下此函数
    '''
    rmdir(packagedir + '.github')
    rmdir(packagedir + 'src')
    rmdir(packagedir + '3rdparty')
    rmdir(packagedir + 'branding')
    rmdir(packagedir + 'db/import')
    rmdir(packagedir + 'conf/import')
    rmdir(packagedir + 'doc/model')
    rmdir(packagedir + 'npc/test')
    rmdir(packagedir + 'db/dicts')
    
    recursive_overwrite(packagedir + 'tools/python/dist/', packagedir)
    rmdir(packagedir + 'tools', ['batches'])
    
    remove_files(packagedir + 'doc', 'packet_*.txt')
    remove_files(packagedir, '*.sh')
    remove_files(packagedir, '*.in')
    remove_files(packagedir, '*.sln')
    remove_files(packagedir, '.*')
    remove_files(packagedir, '*.scpt')
    remove_files(packagedir, '*.yml')
    remove_files(packagedir, '*.ruleset')
    
    remove_file(packagedir + 'doc', 'source_doc.txt')
    remove_file(packagedir + 'npc', 'scripts_test.conf')
    remove_file(packagedir + 'conf', 'valkyrie_sample.cfg')
    remove_file(packagedir, 'AUTHORS')
    remove_file(packagedir, 'LICENSE')
    remove_file(packagedir, 'configure')
    remove_file(packagedir, 'athena-start')
    remove_file(packagedir, 'CMakeLists.txt')
    remove_file(packagedir, 'Jenkinsfile')

    remove_file(packagedir, 'DONATION.md')
    remove_file(packagedir, 'README.md')
    remove_file(packagedir, 'CHANGELOG_FULL.md')
    os.rename(packagedir + 'CHANGELOG.md', packagedir + 'changelog.md')
    
    copyfile(packagedir + 'sql-files/upgrades/premium_storage.sql', packagedir + 'sql-files/premium_storage.sql')
    rmdir(packagedir + 'sql-files/tools')
    rmdir(packagedir + 'sql-files/upgrades')
    
    shutil.move(packagedir + 'tools/batches/runserver.bat', packagedir + 'runserver.bat')
    shutil.move(packagedir + 'tools/batches/navigenerator.bat', packagedir + 'navigenerator.bat')
    
    # --------------------------------------------------------
    # 对数据库的创建脚本进行分类归档
    # --------------------------------------------------------
    
    # 主数据库
    os.makedirs(packagedir + 'sql-files/main/creation')
    shutil.move(packagedir + 'sql-files/main.sql', packagedir + 'sql-files/main/creation/01.main.sql')
    shutil.move(packagedir + 'sql-files/roulette_default_data.sql', packagedir + 'sql-files/main/creation/02.roulette_default_data.sql')
    
    os.makedirs(packagedir + 'sql-files/main/creation/use_sql_db/renewal')
    shutil.move(packagedir + 'sql-files/item_db_re.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/01.item_db_re.sql')
    shutil.move(packagedir + 'sql-files/item_db2_re.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/02.item_db2_re.sql')
    shutil.move(packagedir + 'sql-files/item_db_re_equip.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/03.item_db_re_equip.sql')
    shutil.move(packagedir + 'sql-files/item_db_re_etc.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/04.item_db_re_etc.sql')
    shutil.move(packagedir + 'sql-files/item_db_re_usable.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/05.item_db_re_usable.sql')
    shutil.move(packagedir + 'sql-files/mob_db_re.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/06.mob_db_re.sql')
    shutil.move(packagedir + 'sql-files/mob_db2_re.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/07.mob_db2_re.sql')
    shutil.move(packagedir + 'sql-files/mob_skill_db_re.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/08.mob_skill_db_re.sql')
    shutil.move(packagedir + 'sql-files/mob_skill_db2_re.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/09.mob_skill_db2_re.sql')
    os.makedirs(packagedir + 'sql-files/main/creation/use_sql_db/renewal/optional')
    shutil.move(packagedir + 'sql-files/compatibility/item_db_re_compat.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/optional/01.item_db_re_compat.sql')
    shutil.move(packagedir + 'sql-files/compatibility/item_db2_re_compat.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/optional/02.item_db2_re_compat.sql')
    
    os.makedirs(packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal')
    shutil.move(packagedir + 'sql-files/item_db.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/01.item_db.sql')
    shutil.move(packagedir + 'sql-files/item_db2.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/02.item_db2.sql')
    shutil.move(packagedir + 'sql-files/item_db_equip.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/03.item_db_equip.sql')
    shutil.move(packagedir + 'sql-files/item_db_etc.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/04.item_db_etc.sql')
    shutil.move(packagedir + 'sql-files/item_db_usable.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/05.item_db_usable.sql')
    shutil.move(packagedir + 'sql-files/mob_db.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/06.mob_db.sql')
    shutil.move(packagedir + 'sql-files/mob_db2.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/07.mob_db2.sql')
    shutil.move(packagedir + 'sql-files/mob_skill_db.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/08.mob_skill_db.sql')
    shutil.move(packagedir + 'sql-files/mob_skill_db2.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/09.mob_skill_db2.sql')
    os.makedirs(packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/optional')
    shutil.move(packagedir + 'sql-files/compatibility/item_db_compat.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/optional/01.item_db_compat.sql')
    shutil.move(packagedir + 'sql-files/compatibility/item_db2_compat.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/optional/02.item_db2_compat.sql')
    
    os.makedirs(packagedir + 'sql-files/main/creation/optional')
    shutil.move(packagedir + 'sql-files/premium_storage.sql', packagedir + 'sql-files/main/creation/optional/premium_storage.sql')
    
    # 判断当前是否为专业版
    subdir = 'professional' if is_commercial else 'community'
    
    if os.path.exists(packagedir + f'sql-files/composer/{subdir}/main'):
        shutil.move(packagedir + f'sql-files/composer/{subdir}/main', packagedir + 'sql-files/main/upgrades')
    
    # 日志数据库
    os.makedirs(packagedir + 'sql-files/logs/creation')
    shutil.move(packagedir + 'sql-files/logs.sql', packagedir + 'sql-files/logs/creation/01.logs.sql')
    
    if os.path.exists(packagedir + f'sql-files/composer/{subdir}/logs'):
        shutil.move(packagedir + f'sql-files/composer/{subdir}/logs', packagedir + 'sql-files/logs/upgrades')
        
    # WEB 接口数据库
    os.makedirs(packagedir + 'sql-files/web/creation')
    shutil.move(packagedir + 'sql-files/web.sql', packagedir + 'sql-files/web/creation/01.web.sql')
    
    if os.path.exists(packagedir + f'sql-files/composer/{subdir}/web'):
        shutil.move(packagedir + f'sql-files/composer/{subdir}/web', packagedir + 'sql-files/web/upgrades')
    
    # 清理工作
    rmdir(packagedir + 'sql-files/composer')
    rmdir(packagedir + 'sql-files/compatibility')

def arrange_renewal(packagedir):
    '''
    执行复兴后版本的整理工作
    '''
    arrange_common(packagedir)

    rmdir(packagedir + 'db/pre-re')
    rmdir(packagedir + 'npc/pre-re')

    copyfiles(project_slndir, '*.dll', packagedir)

    copyfile(project_slndir + 'login-server.exe', packagedir + 'login-server.exe')
    copyfile(project_slndir + 'char-server.exe', packagedir + 'char-server.exe')
    copyfile(project_slndir + 'map-server.exe', packagedir + 'map-server.exe')
    copyfile(project_slndir + 'map-server-generator.exe', packagedir + 'map-server-generator.exe')
    copyfile(project_slndir + 'web-server.exe', packagedir + 'web-server.exe')
    copyfile(project_slndir + 'csv2yaml.exe', packagedir + 'csv2yaml.exe')
    copyfile(project_slndir + 'mapcache.exe', packagedir + 'mapcache.exe')
    copyfile(project_slndir + 'yaml2sql.exe', packagedir + 'yaml2sql.exe')
    copyfile(project_slndir + 'yamlupgrade.exe', packagedir + 'yamlupgrade.exe')
    
    if is_commercial and Common.is_file_exists(project_slndir + 'map-server.protected.exe'):
        copyfile(project_slndir + 'map-server.protected.exe', packagedir + 'map-server.exe')

def arrange_pre_renewal(packagedir):
    '''
    执行复兴前版本的整理工作
    '''
    arrange_common(packagedir)

    rmdir(packagedir + 'db/re')
    rmdir(packagedir + 'npc/re')

    copyfiles(project_slndir, '*.dll', packagedir)

    copyfile(project_slndir + 'login-server-pre.exe', packagedir + 'login-server.exe')
    copyfile(project_slndir + 'char-server-pre.exe', packagedir + 'char-server.exe')
    copyfile(project_slndir + 'map-server-pre.exe', packagedir + 'map-server.exe')
    copyfile(project_slndir + 'map-server-generator-pre.exe', packagedir + 'map-server-generator.exe')
    copyfile(project_slndir + 'web-server-pre.exe', packagedir + 'web-server.exe')
    copyfile(project_slndir + 'csv2yaml-pre.exe', packagedir + 'csv2yaml.exe')
    copyfile(project_slndir + 'mapcache.exe', packagedir + 'mapcache.exe')
    copyfile(project_slndir + 'yaml2sql-pre.exe', packagedir + 'yaml2sql.exe')
    copyfile(project_slndir + 'yamlupgrade-pre.exe', packagedir + 'yamlupgrade.exe')
    
    if is_commercial and Common.is_file_exists(project_slndir + 'map-server.protected.exe'):
        copyfile(project_slndir + 'map-server-pre.protected.exe', packagedir + 'map-server.exe')

def get_package_dir(renewal, langinfo):
    '''
    构建解压的输出目录
    '''
    # 获取当前的版本号
    version = Common.get_pandas_ver(os.path.abspath(project_slndir), 'v')

    # 处理成文件名中的版本部分
    if is_commercial:
        version = version.replace(' Rev.', '_Rev.')
    
    dirname = '{project_name}/{version}/{project_name}_{version}_{timestamp}_{model}_{lang}'.format(
        project_name = parser.get('name'),
        version = version, model = 'RE' if renewal else 'PRE',
        timestamp = Common.timefmt(True), lang = langinfo['dirname']
    )

    return os.path.abspath(os.path.join(project_publishdir, dirname))

def process_sub(export_file, renewal, langinfo):
    print('')

    Message.ShowStatus('正在准备生成 {model} - {lang} 的打包目录...'.format(
        model = '复兴后(RE)' if renewal else '复兴前(PRE)',
        lang = langinfo['name']
    ))

    # 构建解压的输出目录
    packagedir = get_package_dir(renewal, langinfo)

    # 获取压缩文件的保存路径
    zipfilename = packagedir + '.zip'

    # 打包目录的末尾追加一个斜杠
    packagedir = packagedir + os.path.sep
    
    # 确保目标文件夹存在
    os.makedirs(os.path.dirname(packagedir), exist_ok = True)
    
    # 若之前目录已经存在, 先删掉
    if os.path.exists(packagedir) and os.path.isdir(packagedir):
        shutil.rmtree(packagedir)
    
    # 将 zip 文件解压到指定的目录中去
    Message.ShowStatus('正在解压归档文件到: %s' % os.path.relpath(
        packagedir, os.path.abspath(os.path.join(packagedir, r'../../'))
    ))
    if not zip_unpack(export_file, packagedir):
        clean(export_file)
        Message.ShowError('很抱歉, 解压归档文件失败, 程序终止.')
        Common.exit_with_pause(-1)
    
    # 进行文本的翻译工作
    trans.process(packagedir, langinfo['trans'], True, True)
    
    # 进行后期处理
    Message.ShowStatus('正在对打包源目录进行后期处理...')
    if renewal:
        arrange_renewal(packagedir)
    else:
        arrange_pre_renewal(packagedir)

    # 专业版的特殊处理逻辑
    remove_file(packagedir, 'VMProtectSDK32.dll')
    remove_file(packagedir, 'VMProtectSDK64.dll')
    rmdir(packagedir + 'secret')

    Message.ShowStatus('后期处理完毕, 即将把打包源压缩成 ZIP 文件...')
    
    # 执行打包操作
    password = parser.get_publish('zip_password')
    if not zip_pack(packagedir, zipfilename, password):
        clean(export_file)
        Message.ShowError('打包成 zip 文件时失败了, 请联系开发者协助定位问题, 程序终止.')
        Common.exit_with_pause(-1)

    Message.ShowStatus('已成功构建 {model} - {lang} 的压缩文件.'.format(
        model = '复兴后(RE)' if renewal else '复兴前(PRE)',
        lang = langinfo['name']
    ))

    # 若处于 Jenkins 环境下, 则将制品复制到指定的目录中去
    if Common.is_jenkins():
        os.makedirs(os.path.join(
            os.environ['WORKSPACE'], 'artifacts', 'packages'
        ), exist_ok = True)
        
        copyfile(zipfilename, os.path.join(
            os.environ['WORKSPACE'], 'artifacts', 'packages', os.path.basename(zipfilename)
        ))
        Message.ShowStatus('已将打包后的 ZIP 文件复制到制品输出目录中.')

def process(export_file, renewal, publish_lang):
    '''
    开始进行处理工作
    '''
    if 'gbk' in publish_lang:
        process_sub(
            export_file = export_file,
            renewal = renewal,
            langinfo = {
                'trans' : 'zh-cn',
                'dirname' : 'GBK',
                'name' : '简体中文'
            }
        )
    
    if 'big5' in publish_lang:
        process_sub(
            export_file = export_file,
            renewal = renewal,
            langinfo = {
                'trans' : 'zh-tw',
                'dirname' : 'BIG5',
                'name' : '繁体中文'
            }
        )

def clean(export_file):
    '''
    执行一些清理工作
    '''
    Message.ShowStatus('正在进行一些善后清理工作...')
    if os.path.exists(export_file):
        os.remove(export_file)

def do_publish(compile_mode, publish_lang):
    # 读取并展现当前熊猫模拟器的版本号
    project_name = parser.get('name')
    Message.ShowInfo('当前输出的项目名称为: %s' % project_name)
    Common.display_version_info(slndir_path, repo)
    Message.ShowInfo('本次需要打包的模式为: %s' % compile_mode + ' | 打包语言: %s' % publish_lang)
    Message.ShowInfo('打包输出目录: %s' % project_publishdir)

    # 检查是否已经完成了编译
    if 're' in compile_mode:
        if not Common.is_compiled(project_slndir, checkmodel='re'):
            Message.ShowWarning('检测到打包需要的编译产物不完整, 请重新编译. 程序终止.')
            print('')
            Common.exit_with_pause(-1)

    if 'pre' in compile_mode:
        if not Common.is_compiled(project_slndir, checkmodel='pre'):
            Message.ShowWarning('检测到打包需要的编译产物不完整, 请重新编译. 程序终止.')
            print('')
            Common.exit_with_pause(-1)

    # 导出当前仓库, 变成一个归档压缩文件
    Message.ShowInfo('正在将当前分支的 HEAD 内容导出成归档文件...')
    export_file = export()
    if not export_file:
        Message.ShowError('很抱歉, 导出归档文件失败, 程序终止.')
        Common.exit_with_pause(-1)
    Message.ShowInfo('归档文件导出完成, 此文件将在程序结束时被删除.') 

    # 基于归档压缩文件, 进行打包处理
    if 're' in compile_mode:
        process(export_file, renewal=True, publish_lang=publish_lang)
    
    if 'pre' in compile_mode:
        process(export_file, renewal=False, publish_lang=publish_lang)

    # 执行一些清理工作
    clean(export_file)

    print('')
    Message.ShowInfo('已经成功打包相关文件, 请进行人工核验.')

def main():
    '''
    主入口函数
    '''
    # 显示欢迎信息
    Common.welcome('打包流程辅助脚本')

    # 只能在 Windows 环境运行
    if platform.system() != 'Windows':
        Message.ShowError('很抱歉, 此脚本只能在 Windows 环境上运行')
        Common.exit_with_pause(-1)
    
    print('')

    # 读取当前的编译模式配置
    compile_mode = parser.get('compile_mode')

    # 读取当前的发布语言配置
    publish_lang = parser.get('publish_lang')

    # 根据配置执行打包
    do_publish(compile_mode, publish_lang)

    # 友好退出, 主要是在 Windows 环境里给予暂停
    Common.exit_with_pause()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt as _err:
        pass
