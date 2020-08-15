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

import os
import git
import zipfile
import glob
import shutil
import pyhelp_translate as trans

from libs import Common, Message

# 切换工作目录为脚本所在目录
os.chdir(os.path.split(os.path.realpath(__file__))[0])

# 工程文件的主目录相对此脚本文件的位置
project_slndir = '../../'

def export():
    '''
    将当前工程目录的全部内容导出成一个 zip 文件
    '''
    repo = git.Repo(project_slndir)
    export_path = os.path.abspath(project_slndir + 'pandas_export.zip')
    repo.git.archive('HEAD', '--format=zip', '-o', export_path)
    return export_path if os.path.exists(export_path) else None

def zip_unpack(zipfilename, targetdir):
    '''
    将一个 zip 文件解压缩到指定的目录
    '''
    try:
        zip = zipfile.ZipFile(zipfilename, 'r')
        zip.extractall(targetdir)
        zip.close()
    except Exception as _err:
        return False
    else:
        return True

def zip_pack(sourcedir, zipfilename):
    '''
    将一个指定的目录打包成指定的路径 zip 文件
    '''
    try:
        basename = os.path.basename(os.path.normpath(sourcedir))
        z = zipfile.ZipFile(zipfilename, 'w', zipfile.ZIP_DEFLATED)
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

    remove_file(packagedir, 'DONATION.md')
    remove_file(packagedir, 'README.md')
    os.rename(packagedir + 'CHANGELOG.md', packagedir + 'changelog.md')
    
    copyfile(packagedir + 'sql-files/upgrades/premium_storage.sql', packagedir + 'sql-files/premium_storage.sql')
    rmdir(packagedir + 'sql-files/tools')
    rmdir(packagedir + 'sql-files/upgrades')
    
    copyfile(packagedir + 'tools/batches/runserver.bat', packagedir + 'renewal.bat')
    remove_file(packagedir + 'tools/batches', 'runserver.bat')
    
    # --------------------------------------------------------
    # 对数据库的创建脚本进行分类归档
    # --------------------------------------------------------
    
    # 主数据库
    os.makedirs(packagedir + 'sql-files/main/creation')
    shutil.move(packagedir + 'sql-files/main.sql', packagedir + 'sql-files/main/creation/01.main.sql')
    shutil.move(packagedir + 'sql-files/roulette_default_data.sql', packagedir + 'sql-files/main/creation/02.roulette_default_data.sql')
    
    os.makedirs(packagedir + 'sql-files/main/creation/use_sql_db/common')
    shutil.move(packagedir + 'sql-files/item_cash_db.sql', packagedir + 'sql-files/main/creation/use_sql_db/common/01.item_cash_db.sql')
    shutil.move(packagedir + 'sql-files/item_cash_db2.sql', packagedir + 'sql-files/main/creation/use_sql_db/common/02.item_cash_db2.sql')
    
    os.makedirs(packagedir + 'sql-files/main/creation/use_sql_db/renewal')
    shutil.move(packagedir + 'sql-files/item_db_re.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/01.item_db_re.sql')
    shutil.move(packagedir + 'sql-files/item_db2_re.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/02.item_db2_re.sql')
    shutil.move(packagedir + 'sql-files/mob_db_re.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/03.mob_db_re.sql')
    shutil.move(packagedir + 'sql-files/mob_db2_re.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/04.mob_db2_re.sql')
    shutil.move(packagedir + 'sql-files/mob_skill_db_re.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/05.mob_skill_db_re.sql')
    shutil.move(packagedir + 'sql-files/mob_skill_db2_re.sql', packagedir + 'sql-files/main/creation/use_sql_db/renewal/06.mob_skill_db2_re.sql')
    
    os.makedirs(packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal')
    shutil.move(packagedir + 'sql-files/item_db.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/01.item_db.sql')
    shutil.move(packagedir + 'sql-files/item_db2.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/02.item_db2.sql')
    shutil.move(packagedir + 'sql-files/mob_db.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/03.mob_db.sql')
    shutil.move(packagedir + 'sql-files/mob_db2.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/04.mob_db2.sql')
    shutil.move(packagedir + 'sql-files/mob_skill_db.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/05.mob_skill_db.sql')
    shutil.move(packagedir + 'sql-files/mob_skill_db2.sql', packagedir + 'sql-files/main/creation/use_sql_db/pre-renewal/06.mob_skill_db2.sql')
    
    os.makedirs(packagedir + 'sql-files/main/creation/optional')
    shutil.move(packagedir + 'sql-files/premium_storage.sql', packagedir + 'sql-files/main/creation/optional/premium_storage.sql')
    
    if os.path.exists(packagedir + 'sql-files/composer/main'):
        shutil.move(packagedir + 'sql-files/composer/main', packagedir + 'sql-files/main/upgrades')
    
    # 日志数据库
    os.makedirs(packagedir + 'sql-files/logs/creation')
    shutil.move(packagedir + 'sql-files/logs.sql', packagedir + 'sql-files/logs/creation/01.logs.sql')
    
    if os.path.exists(packagedir + 'sql-files/composer/logs'):
        shutil.move(packagedir + 'sql-files/composer/logs', packagedir + 'sql-files/logs/upgrades')
        
    rmdir(packagedir + 'sql-files/composer')

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
    copyfile(project_slndir + 'csv2yaml.exe', packagedir + 'csv2yaml.exe')
    copyfile(project_slndir + 'mapcache.exe', packagedir + 'mapcache.exe')

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
    copyfile(project_slndir + 'csv2yaml.exe', packagedir + 'csv2yaml.exe')
    copyfile(project_slndir + 'mapcache.exe', packagedir + 'mapcache.exe')

def process_sub(export_file, renewal, langinfo):
    print('')
    
    # 确认当前的版本号
    version = Common.get_pandas_ver(os.path.abspath(project_slndir), 'v')

    Message.ShowStatus('正在准备生成 {model} - {lang} 的打包目录...'.format(
        model = '复兴后(RE)' if renewal else '复兴前(PRE)',
        lang = langinfo['name']
    ))

    # 构建解压的打包目录
    packagedir = '../Release/Pandas/{version}/Pandas_{version}_{timestamp}_{model}_{lang}'.format(
        version = version, model = 'RE' if renewal else 'PRE',
        timestamp = Common.timefmt(True), lang = langinfo['dirname']
    )

    # 获取压缩文件的保存路径
    zipfilename = os.path.abspath(project_slndir + packagedir) + '.zip'

    # 获取打包的绝对路径
    packagedir = os.path.abspath(project_slndir + packagedir) + os.path.sep
    
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
    trans.process(packagedir, langinfo['trans'], True)
    
    # 进行后期处理
    Message.ShowStatus('正在对打包源目录进行后期处理...')
    if renewal:
        arrange_renewal(packagedir)
    else:
        arrange_pre_renewal(packagedir)
    Message.ShowStatus('后期处理完毕, 即将把打包源压缩成 ZIP 文件...')
    
    # 执行打包操作
    if not zip_pack(packagedir, zipfilename):
        clean(export_file)
        Message.ShowError('打包成 zip 文件时失败了, 请联系开发者协助定位问题, 程序终止.')
        Common.exit_with_pause(-1)

    Message.ShowStatus('已成功构建 {model} - {lang} 的压缩文件.'.format(
        model = '复兴后(RE)' if renewal else '复兴前(PRE)',
        lang = langinfo['name']
    ))

def process(export_file, renewal):
    '''
    开始进行处理工作
    '''
    process_sub(
        export_file = export_file,
        renewal = renewal,
        langinfo = {
            'trans' : 'zh-cn',
            'dirname' : 'GBK',
            'name' : '简体中文'
        }
    )
    
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

def main():
    '''
    主入口函数
    '''
    # 显示欢迎信息
    Common.welcome('打包流程辅助脚本')
    print('')

    pandas_ver = Common.get_pandas_ver(os.path.abspath(project_slndir))
    Message.ShowInfo('当前模拟器的主版本是 %s' % pandas_ver)

    # 检查是否已经完成了编译
    if not Common.is_compiled(project_slndir):
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
    process(export_file, renewal=True)
    process(export_file, renewal=False)

    # 执行一些清理工作
    clean(export_file)

    print('')
    Message.ShowInfo('已经成功打包相关文件, 请进行人工核验.')

    # 友好退出, 主要是在 Windows 环境里给予暂停
    Common.exit_with_pause()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt as _err:
        pass
