import os
import sys
import configparser
import json
import subprocess

STR_REPORT_PATH = ''
STR_COMPILE_LOG_PATH = ''
STR_FORMAT = 'csv'

STR_WARNING = 'warning:'
STR_TIME_MAKE = 'time make'
STR_FILE_NAME = 'file'
STR_ERROR_LINE = 'err_line'
STR_WARNING_MSG = 'warning_msg'
STR_WARNING_TYPE_UNDEFINE = 'type undefine'

G_RET_VALUE = 0
#== function ==
def get_report_dir_path(in_path):
    path_arr = in_path.split('/')
    out_path = ''
    for dir_name in path_arr:
        if path_arr.index(dir_name) < len(path_arr) -1:
            out_path +=  dir_name + '/'
    
    return out_path

def check_input_path_is_exist():
    if STR_COMPILE_LOG_PATH == '':
        print('find path value is NULL, please to use "--COMPILE_LOG_PATH" to setting the value.')
        sys.exit(1)
    if not os.path.exists(STR_COMPILE_LOG_PATH):
        print('find path error: %s \n' % STR_COMPILE_LOG_PATH)
        sys.exit(1)
        
    report_dir = get_report_dir_path(STR_REPORT_PATH)
    if STR_REPORT_PATH != '' and not os.path.exists(report_dir):
        print('report path error: %s \n' % STR_REPORT_PATH)
        sys.exit(1)
        
def create_section_dict(out_section_dict):
    tmp_dict = {}
    tmp_section = {}
    tmp_count = 0
    with open(STR_COMPILE_LOG_PATH) as f:
        for line in f.readlines():
            time_make_str_exist = STR_TIME_MAKE in line
            warning_str_exist = STR_WARNING in line
            if time_make_str_exist == True:
                tmp_section = line.strip()
                tmp_dict[tmp_section] = {}
                tmp_count = 0
                # print(tmp_section)
            elif warning_str_exist == True:
                tmp_dict[tmp_section][tmp_count] = line
                tmp_count += 1
    # print(tmp_dict)
    out_section_dict.update(tmp_dict);

def create_warning_dict(in_section_dict, out_warning_dict):
    #[warning type][section][file name][error line][address][warning msg]
    tmp_dict = {}
    for section_index, section_array in in_section_dict.items(): 
        # print('data_index:%s , data_array:%s' % (data_index, data_array) )
        for data_index in section_array:
            tmp_warning_type = ''
            # print(section_array[data_index])
            symbol_exist = '[-' in section_array[data_index]
            if symbol_exist == True:
                tmp_warning_type_array = section_array[data_index].split('[-')
                tmp_warning_type_array = tmp_warning_type_array[1].split(']')
                tmp_warning_type = tmp_warning_type_array[0]
            else:
                tmp_warning_type = STR_WARNING_TYPE_UNDEFINE
                
            tmp_warning_array = section_array[data_index].split(':')
            tmp_warning_msg = ""
            for index in range(len(tmp_warning_array)):
                # print(index)
                if index >3:
                    tmp_warning_msg += tmp_warning_array[index]
            
            warning_type_exist = tmp_warning_type in tmp_dict
            if warning_type_exist == False:
                tmp_dict[tmp_warning_type] = {}
            
                
            # print(tmp_dict)
            # print(warning_type_exist)
            
            tmp_err_line_dict = {}
            tmp_warning_file_dict = {}
            tmp_section_dict = {}
            
            if len(tmp_warning_array) < 5:
                print(tmp_warning_array)
            else:
                section_exist = section_index in tmp_dict[tmp_warning_type]
                if section_exist == False:
                    tmp_dict[tmp_warning_type][section_index] = {}
                    
                file_warning_exist = tmp_warning_array[0] in tmp_dict[tmp_warning_type][section_index]
                if file_warning_exist == False:
                    tmp_dict[tmp_warning_type][section_index][tmp_warning_array[0]] = {}
                    
                file_warning_addr_exist = tmp_warning_array[1] in tmp_dict[tmp_warning_type][section_index][tmp_warning_array[0]]
                if file_warning_addr_exist == False:
                    tmp_dict[tmp_warning_type][section_index][tmp_warning_array[0]][tmp_warning_array[1]] = {}
                    
                tmp_dict[tmp_warning_type][section_index][tmp_warning_array[0]][tmp_warning_array[1]][tmp_warning_array[2]] = tmp_warning_msg
                # tmp_err_line_dict[ tmp_warning_array[1] ] = tmp_warning_array[4]
                # tmp_warning_file_dict[ tmp_warning_array[0] ] = tmp_err_line_dict
                # tmp_section_dict[ section_index ] = tmp_warning_file_dict
                # tmp_dict[tmp_warning_type].update(tmp_section_dict)
    out_warning_dict.update(tmp_dict);
    
def create_section_warning_dict(in_section_dict, out_warning_dict):
    #[section][file name][error line][address][warning type][warning msg]
    tmp_dict = {}
    for section_index, section_array in in_section_dict.items(): 
        # print('data_index:%s , data_array:%s' % (data_index, data_array) )
        for data_index in section_array:
            tmp_warning_type = ''
            # print(section_array[data_index])
            symbol_exist = '[-' in section_array[data_index]
            if symbol_exist == True:
                tmp_warning_type_array = section_array[data_index].split('[-')
                tmp_warning_type_array = tmp_warning_type_array[1].split(']')
                tmp_warning_type = tmp_warning_type_array[0]
            else:
                tmp_warning_type = STR_WARNING_TYPE_UNDEFINE
                
            tmp_warning_array = section_array[data_index].split(':')
            tmp_warning_msg = ""
            for index in range(len(tmp_warning_array)):
                # print(index)
                if index >3:
                    tmp_warning_msg += tmp_warning_array[index]
            
            # warning_type_exist = tmp_warning_type in tmp_dict
            # if warning_type_exist == False:
                # tmp_dict[tmp_warning_type] = {}
            
            if len(tmp_warning_array) < 5:
                print(tmp_warning_array)
            else:
                section_exist = section_index in tmp_dict
                if section_exist == False:
                    tmp_dict[section_index] = {}
                    
                file_exist = tmp_warning_array[0] in tmp_dict[section_index]
                if file_exist == False:
                    tmp_dict[section_index][tmp_warning_array[0]] = {}
                    
                line_warning_exist = tmp_warning_array[1] in tmp_dict[section_index][tmp_warning_array[0]]
                if line_warning_exist == False:
                    tmp_dict[section_index][tmp_warning_array[0]][tmp_warning_array[1]] = {}
                
                address_warning_exist = tmp_warning_array[2] in tmp_dict[section_index][tmp_warning_array[0]][tmp_warning_array[1]]
                if address_warning_exist == False:
                    tmp_dict[section_index][tmp_warning_array[0]][tmp_warning_array[1]][tmp_warning_array[2]] = {}
                    
                tmp_dict[section_index][tmp_warning_array[0]][tmp_warning_array[1]][tmp_warning_array[2]][0] = tmp_warning_type
                tmp_dict[section_index][tmp_warning_array[0]][tmp_warning_array[1]][tmp_warning_array[2]][1] = tmp_warning_msg
                
    out_warning_dict.update(tmp_dict);
            
# def gen_normal_report(in_warning_dict):
    # tmp_write_array = []
    # for warning_type_index, section_data in in_warning_dict.items():
        # tmp_string = "---[" + warning_type_index + "]"
        # tmp_write_array.append(tmp_string)
        # for section_index in section_data:
            # tmp_string = "==== <" + section_index + "> ===="
            # tmp_write_array.append(tmp_string)
            # for file_index in section_data[section_index]:
                # file_data = section_data[section_index][file_index]
                # for line_index in file_data:
                    # line_data = file_data[line_index]
                    # for address_index in line_data:
                        # warning_msg = line_data[address_index]
                        # tmp_string = file_index + ":" + line_index + ":" + address_index + ":" + warning_msg
                        # tmp_write_array.append(tmp_string)
    # with open(STR_REPORT_PATH, 'w') as f:
        # for line in tmp_write_array:
            # f.write(line)
            # f.write("\n")
    
# def gen_csv_report(in_warning_dict):
    # tmp_write_array = []
    # for warning_type_index, section_data in in_warning_dict.items():
        # for section_index in section_data:
            # for file_index in section_data[section_index]:
                # file_data = section_data[section_index][file_index]
                # for line_index in file_data:
                    # line_data = file_data[line_index]
                    # for address_index in line_data:
                        # warning_msg = line_data[address_index]
                        # tmp_string = warning_type_index + '`' + section_index + '`' +file_index + "`" + line_index + "`" + address_index + "`" + warning_msg
                        # tmp_write_array.append(tmp_string)
    
    
    # with open(STR_REPORT_PATH, 'w') as f:
        # for line in tmp_write_array:
            # f.write(line)
    
def gen_csv_report(in_section_dict):
    tmp_write_array = []
    for section_index, section_array in in_section_dict.items():
        for data_index in section_array:
            tmp_warning_type = ''
            symbol_exist = '[-' in section_array[data_index]
            if symbol_exist == True:
                tmp_warning_type_array = section_array[data_index].split('[-')
                tmp_warning_type_array = tmp_warning_type_array[1].split(']')
                tmp_warning_type = tmp_warning_type_array[0]
            else:
                tmp_warning_type = STR_WARNING_TYPE_UNDEFINE
                
            tmp_warning_array = section_array[data_index].split(':')
            tmp_warning_msg = ""
            for index in range(len(tmp_warning_array)):
                # print(index)
                if index >3:
                    tmp_warning_msg += tmp_warning_array[index]
            
            if len(tmp_warning_array) < 5:
                print(tmp_warning_array)
            else:
                file_name = tmp_warning_array[0]
                line_num = tmp_warning_array[1]
                address_num = tmp_warning_array[2]
                
                tmp_string = section_index + '`' +file_name + "`" + line_num + "`" + address_num + "`"  + tmp_warning_type + '`' + tmp_warning_msg
                tmp_write_array.append(tmp_string)
    
    
    with open(STR_REPORT_PATH, 'w') as f:
        for line in tmp_write_array:
            f.write(line)

def gen_report(in_warning_dict):
    gen_csv_report(in_warning_dict)
    # if STR_FORMAT == 'normal':
        # gen_normal_report(in_warning_dict)
    # elif STR_FORMAT == 'csv':
        # gen_csv_report(in_warning_dict)

#-- main function --
def print_help_msg():
    print('Usage: python3 warning_check.py <PARAM> ...\n')
    print('<PARAM>=\n')
    print('\t--COMPILE_LOG_PATH <FILE>: warning_check.py will parser the <FILE>. \n')
    print('\t--FORMAT <FORMAT>: <FORMAT> have "normal" or "ini". Default is "normal" format.\n')
    print('\t-o <PATH>: Report content will output to <PATH>. If no setting this value. \n')
    print('\t--help: Print help message. \n')
    sys.exit(1)
        
# == main ==
for argv in sys.argv:
# print('%d %s\n' % (sys.argv.index(argv), argv))
    if argv == '-o':
        tmp_index =  sys.argv.index(argv)
        if len(sys.argv) >= tmp_index+1:
            STR_REPORT_PATH = sys.argv[tmp_index+1]
        else:
            print_help_msg()
    elif argv == '--COMPILE_LOG_PATH':
        tmp_index =  sys.argv.index(argv)
        if len(sys.argv) >= tmp_index+1:
            # print('index: %s' % sys.argv[tmp_index+1])
            STR_COMPILE_LOG_PATH = sys.argv[tmp_index+1]
            # print('STR_CHECK_ROOT_PATH: %s' % STR_CHECK_ROOT_PATH)
        else:
            print_help_msg()
    elif argv == '--FORMAT':
        tmp_index =  sys.argv.index(argv)
        if len(sys.argv) >= tmp_index+1:
            # if sys.argv[tmp_index+1] == 'normal' or sys.argv[tmp_index+1] == 'csv':
            if sys.argv[tmp_index+1] == 'csv':
                STR_FORMAT = sys.argv[tmp_index+1]
            else:
                print_help_msg()
            # print('STR_CHECK_ROOT_PATH: %s' % STR_CHECK_ROOT_PATH)
        else:
            print_help_msg()
    elif argv == '--help':
        print_help_msg()
        

section_dict = {}
warning_dict = {}      #[warning type][section][file name][error line][address][warning msg]

check_input_path_is_exist();
create_section_dict(section_dict);
gen_report(section_dict);
