/*******************************************\
* Copyright (C) SDMC
* Author: feeman
* File: Task.h
\*******************************************/

#ifndef __TASK_H__
#define __TASK_H__

#include "OSTypeDef.h"
#include <iostream>
#include <string>
using namespace std;

class Task {
public:

    UInt32 m_u32Taskid;  // [Y]
    string m_strAlbumName;  // [Y]
    string m_strVideoName; // tell the file name [Y]
    UInt32 m_u32Priority; // tell the priority [Y]
    UInt64 m_u64FileSize; // tell the total size of the movie [Y]
    string m_strVideoUrl; // tell where to get the video file. [Y]
    string m_strAlbumVerPic; // tell where to get the vertical picture, [O]
    string m_strAlbumExcel; // tell where to get the excel list. [O]

    Task() : m_strAlbumVerPic(""), m_strAlbumExcel("") {}

    void ShowTask()
    {
        cerr << "ShowTask Begin========================>" << endl;
        cerr << "Task ID: " << m_u32Taskid << endl;
        cerr << "Album name: " << m_strAlbumName << endl;
        cerr << "Vedio Name: " << m_strVideoName << endl;
        cerr << "Priority: " << m_u32Priority << endl;
        cerr << "File size: " << m_u64FileSize << endl;
        cerr << "Vedio url: " << m_strVideoUrl << endl;

        cerr << "Album Vertical Pic: " << m_strAlbumVerPic << endl;
        cerr << "Album Excel: " << m_strAlbumExcel << endl;
    }
};

#if 0
// sort by priority from high to low
class Cmpare
{
public:
    bool operator()(const Task* task1, const Task* task2) const
	{
	    return task1->m_u32Priority > task2->m_u32Priority;
	}
};
#endif

#endif //__TASK_H__
