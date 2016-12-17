//
// Created by cristian on 10.01.2016.
//

#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include <fcntl.h>

#include <sys/sendfile.h>

using namespace std;

void make_directory(const char* username){
    mkdir(username,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}
void copy_file(const char* original_file, const char* copied_file){
    int read_fileDescriptor;
    int write_fileDesctriptor;
    struct stat fileInfo;
    off_t offset = 0;

    //Deschidem fisierul
    read_fileDescriptor = open(original_file, O_RDONLY);
    //Preluam datele despre fisier
    fstat(read_fileDescriptor, &fileInfo);
    //Deschidem fisierul al doilea
    write_fileDesctriptor = open(copied_file,O_WRONLY | O_CREAT, fileInfo.st_mode);
    //Copiem fisierul
    sendfile(write_fileDesctriptor,read_fileDescriptor,&offset,fileInfo.st_size);
    //Inchidem fisierele
    close(read_fileDescriptor);
    close(write_fileDesctriptor);
}
int delete_directory(const char*directory_Name){
    DIR *directory_pointer;
    struct dirent *directory_Content;

    char file_Path[FILENAME_MAX];

    directory_pointer = opendir(directory_Name); //deschidem directorul

    if(directory_pointer != NULL){
        //citim din director
        while(directory_Content = readdir(directory_pointer)){
            struct stat fileInfo; //structura pentru informatiile unui director/fisier din directorul principal

            snprintf(file_Path, FILENAME_MAX, "%s/%s", directory_Name, directory_Content->d_name); //creem un path pentru fisierul/directorul citit

            //salvam in variabila fileInfo informatiile fisierului utilizand path-ul sau
            if(lstat(file_Path, &fileInfo )<0){
                perror(file_Path);
            }

            //vedem daca ce am citit este un director sau fisier
            if(S_ISDIR(fileInfo.st_mode)){
                if(strcmp(directory_Content->d_name, ".") && strcmp(directory_Content->d_name, "..")) {
                    printf("%s directory\n", file_Path);
                    delete_directory(file_Path);
                }
            }
            else {
                printf("%s file\n", file_Path);
                remove(file_Path);
            }
        }
        (void) closedir(directory_pointer);
    }
    else{
        perror ("Couldn't open the directory");
    }

    remove(directory_Name);
    return 0;
}
int copy_directory(const char* directory_Name, const char* new_directory){
    DIR *directory_pointer;
    DIR *directory_pointer2;
    make_directory(new_directory);

    struct dirent *directory_Content;
    char file_Path[FILENAME_MAX];
    char file_Path2[FILENAME_MAX];


    directory_pointer = opendir(directory_Name); //deschidem directorul principal
    directory_pointer2 = opendir(new_directory); //deschidem directorul in care copiem


    if(directory_pointer != NULL){
        //citim din director
        while(directory_Content = readdir(directory_pointer)){
            struct stat fileInfo; //structura pentru informatiile unui director/fisier din directorul principal

            snprintf(file_Path, FILENAME_MAX, "%s/%s", directory_Name, directory_Content->d_name); //creem un path pentru fisierul/directorul citit
            snprintf(file_Path2, FILENAME_MAX, "%s/%s", new_directory, directory_Content->d_name); //creem un path pentru noul fisier

            //salvam in variabila fileInfo informatiile fisierului utilizand path-ul sau
            if(lstat(file_Path, &fileInfo )<0){
                perror(file_Path);
            }

            //vedem daca ce am citit este un director sau fisier
            if(S_ISDIR(fileInfo.st_mode)){
                if(strcmp(directory_Content->d_name, ".") && strcmp(directory_Content->d_name, "..")) {
                    printf("%s directory\n", file_Path);
                    make_directory(file_Path2);
                    copy_directory(file_Path,file_Path2);
                }
            }
            else {
                printf("%s file\n", file_Path);
                copy_file(file_Path,file_Path2);
            }
        }
        (void) closedir(directory_pointer);
    }
    else{
        perror ("Couldn't open the directory");
    }
    return 0;
}
string listing(const char* directory){
    DIR* directory_pointer;
    struct dirent *directory_content;
    string file_Path="\n[server]Listing directory ";
    file_Path+=directory;
    file_Path+=":\n";

    //Deschidem directorul
    directory_pointer = opendir(directory);

    if(directory_pointer != NULL){
        //Listam continutul

        while((directory_content = readdir(directory_pointer)) != NULL){
            if(!strcmp(directory_content->d_name,".") || !strcmp(directory_content->d_name,".."))
                continue;
            file_Path+=directory;
            file_Path+='/';
            file_Path+=directory_content->d_name;
            file_Path+="\n";
        }
        return file_Path;
    }
    else{
        return file_Path;
    }
}