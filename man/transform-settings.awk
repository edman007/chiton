#!/bin/env -S awk -f
# This script transforms c format setting.hpp data to a man file
# We write to the array 'settings', which is key'd by the setting name
#

BEGIN {
    RS=""
    FS="\", "
    last_key = "null"
}

function unescape(input){
    sub(/^[ ]*/, "", input)
    sub(/^\n*"/, "", input)
    #print input
    sub(/"$/, "", input)
    sub(/\\"/, "\"", input)
    sub(/"[ ]*\n[ ]*"/, "", input)
    sub("\n", "", input)
    return input
}

#determines group order
function groupValue(type){
    if (type == "SETTING_READ_ONLY"){
        return 0
    } else if (type == "SETTING_REQUIRED_SYSTEM"){
        return 1
    } else if (type == "SETTING_OPTIONAL_SYSTEM"){
        return 2
    } else if (type == "SETTING_REQUIRED_CAMERA"){
        return 3
    } else if (type == "SETTING_OPTIONAL_CAMERA"){
        return 4
    }
}

#The header of the group
function groupName(type){
    if (type == "SETTING_READ_ONLY"){
        return "Read Only Settings"
    } else if (type == "SETTING_REQUIRED_SYSTEM"){
        return "Required System Settings"
    } else if (type == "SETTING_OPTIONAL_SYSTEM"){
        return "Optional System Settings"
    } else if (type == "SETTING_REQUIRED_CAMERA"){
        return "Required System Settings"
    } else if (type == "SETTING_OPTIONAL_CAMERA"){
        return "Optional Camera Settings"
    }
}

#the description of the group
function groupComment(type){
    if (type == "SETTING_READ_ONLY"){
        return "These can generally only be set in chiton.cfg or as command line parameters and are generally set in typical installations"
    } else if (type == "SETTING_REQUIRED_SYSTEM"){
        return "System settings required to be set for a normal functional system";
    } else if (type == "SETTING_OPTIONAL_SYSTEM"){
        return "System settings not required in all installations";
    } else if (type == "SETTING_REQUIRED_CAMERA"){
        return "Camera settings required to be set for a camera to work"
    } else if (type == "SETTING_OPTIONAL_CAMERA"){
        return "Optional settings that control various features of cameras"
    }

}

function sortSettings(i1, v1, i2, v2, l, r){
    if (v1["type"] == v2["type"]) {
        return v1["key"] > v2["key"]
    } else {
        return groupValue(v1["type"]) > groupValue(v2["type"])
    }
}

#Extract the settings details
/^[\n ]*("|{)/ {
    if (NF == 5){
        key = unescape($1)
        defaultVal = unescape($2)
        name = unescape($3)
        shortHelp = unescape($4)
        type = unescape($5)
        setting[key]["key"] = key
        setting[key]["def"] = defaultVal
        setting[key]["name"] = name
        setting[key]["help"] = shortHelp
        setting[key]["type"] = type
    } else if (NF == 4){
        #assume defaultValu is lacking quotes
        key = unescape($1)
        split($2, splitVal, ", ")
        defaultVal = unescape(splitVal[1])
        name = unescape(splitVal[2])
        shortHelp = unescape($3)
        type = unescape($4)
        setting[key]["key"] = key
        setting[key]["def"] = defaultVal
        setting[key]["name"] = name
        setting[key]["help"] = shortHelp
        setting[key]["type"] = type
    } else {
      print "Formatting error " $0
    }
}

#extract the long text
/^[ ]*\/\*/ {
    keyStart = index($0, "@")
    keyLong = substr($0, keyStart)
    keyEnd = index(keyLong, " ")
    key = substr(keyLong, 2, keyEnd - 2)
    longText = substr(keyLong, keyEnd + 1)
    sub(/\n[ ]*\*/, "", longText)
    setting[key]["long"] = longText
}
END {
    n = asort(setting, sorted, "sortSettings")
    last_type = ""
    for (i = 1; i <= n; i++) {
        if (sorted[i]["type"] != last_type){
            #print header
            last_type = sorted[i]["type"]
            print ".SH " groupName(sorted[i]["type"])
            print groupComment(sorted[i]["type"])
            print ".br"
        }
        print ".br"
        print ".SS " sorted[i]["key"]
        print ".br"
        print ".TR "
        print "Long Name: " sorted[i]["name"]
        print ".br"
        print "Default Value: \"" sorted[i]["def"] "\""
        print ".PP"
        print sorted[i]["help"]
        print ".PP"
        print sorted[i]["long"]
    }

    #print "Complete" setting["motion-cvbackground-tau"]["help"]
}
