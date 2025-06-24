/*
 * =====================================================================================
 *
 *       Filename:  JsonFileManager.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/17/2024 05:35:01 AM
 *       Revision:  none
 *       Compiler:  g++
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */


#ifndef JSON_FILE_MANAGER_H 
#define JSON_FILE_MANAGER_H

#include <LittleFS.h>
#include <Arduino.h>
#include <FS.h>

// typedef enum {
//  settings,
// } Files;



// void init_littlefs();
// bool isDataAvailable(Files f);
// void dump_file2(Files f); 
// bool write_file(uint8_t* data, size_t size, Files f) ;
// bool readData(uint8_t* buffer,size_t* size,Files f) ;
// void stop_littlefs();
// void resetFile(Files f) ;

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) ;
void Dump_file(fs::FS &fs, const char *path) ;



#endif
