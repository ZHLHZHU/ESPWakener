//
// Created by lh on 2021/12/12.
//
#include "validation.h"

/**
 * 检查是否为合法Mac地址,Arduino的正则表达式似乎有点问题
 * @param mac
 * @return
 */
bool isMacAddress(String &mac){
    if(mac.length() != 17){
        return false;
    }
    char separator = mac[2];
    for(int i = 0; i < mac.length(); i++){
        if(i == 2 || i == 5 || i == 8 || i == 11 || i == 14){
            if(mac[i] != ':' && mac[i] != '-'){
                return false;
            }
            if(mac[i] != separator){
                return false;
            }
        }else{
            if(!isHex(mac[i])){
                return false;
            }
        }
    }
    return true;
}

bool isHex(char c){
    if(c >= '0' && c <= '9'){
        return true;
    }
    if(c >= 'a' && c <= 'f'){
        return true;
    }
    if(c >= 'A' && c <= 'F'){
        return true;
    }
    return false;
}