// Authored by AMIT COHEN 315640623
#include <iostream>
#include <vector>
#include <map>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cmath>

using namespace std;

#define DISK_SIZE 256

// didn't use the function in this project
void decToBinary(int n, char &c) {
    // array to store binary number
    int binaryNum[8];

    // counter for binary array
    int i = 0;
    while (n > 0) {
        // storing remainder in binary array
        binaryNum[i] = n % 2;
        n = n / 2;
        i++;
    }

    // printing binary array in reverse order
    for (int j = i - 1; j >= 0; j--) {
        if (binaryNum[j] == 1)
            c = c | 1u << j;
    }
}

// FsFile class contain raw details about the file

class FsFile {
    int file_size; // size
    int block_in_use; // amount
    int index_block; // index start
    int block_size; // size

public:
    FsFile(int _block_size) {
        file_size = 0;
        block_in_use = 0;
        block_size = _block_size;
        index_block = -1;
    }

    // getters and setters

    int getFileSize() const {
        return file_size;
    }

    void setFileSize(int fileSize) {
        file_size = fileSize;
    }

    int getBlockInUse() const {
        return block_in_use;
    }

    void setBlockInUse(int blockInUse) {
        block_in_use = blockInUse;
    }

    int getIndexBlock() const {
        return index_block;
    }

    void setIndexBlock(int indexBlock) {
        index_block = indexBlock;
    }

    int getBlockSize() const {
        return block_size;
    }

    void setBlockSize(int blockSize) {
        block_size = blockSize;
    }

};

// File Descriptor holding the information of the file name and his fsFile

class FileDescriptor {
    string file_name;
    FsFile *fs_file;
    bool inUse;

public:

    FileDescriptor(string FileName, FsFile *fsi) {
        file_name = FileName;
        fs_file = fsi;
        inUse = true;
    }

    string getFileName() {
        return file_name;
    }

    void setFileName(const string &fileName) {
        file_name = fileName;
    }

    FsFile *getFsFile() const {
        return fs_file;
    }

    void setFsFile(FsFile *fsFile) {
        fs_file = fsFile;
    }

    bool isInUse() const {
        return inUse;
    }

    void setInUse(bool inUse) {
        FileDescriptor::inUse = inUse;
    }

};

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"

// Our main disk holding most of the method and the constructor of the disk

class fsDisk {
    FILE *sim_disk_fd;
    bool is_formated;

    // BitVector - "bit" (int) vector, indicate which block in the disk is free
    //              or not.  (i.e. if BitVector[0] == 1 , means that the
    //             first block is occupied.
    int BitVectorSize;
    int *BitVector;

    // (5) MainDir --
    // Structure that links the file name to its FsFile

    vector<FileDescriptor *> MainDir;


    // (6) OpenFileDescriptors --
    //  when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.

    FileDescriptor **OpenFileDescriptors;

    // define essential variables

    int currentBlockSize;

    int maximumSize;

    int amountOfFreeBlocks;

    int sizeCanUse;

public:

    // constructor

    fsDisk() {
        sim_disk_fd = fopen(DISK_SIM_FILE, "r+");
        assert(sim_disk_fd);

        for (int i = 0; i < DISK_SIZE; i++) {
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fwrite("\0", 1, 1, sim_disk_fd);
            assert(ret_val == 1);
        }

        fflush(sim_disk_fd);

        is_formated = false;

        currentBlockSize = 0;

        sizeCanUse = DISK_SIZE;
    }

    virtual ~fsDisk() {
        while (!MainDir.empty()) {
            DelFile(MainDir[0]->getFileName());
        }
        delete[] OpenFileDescriptors;
        delete[] BitVector;
        MainDir.clear();
        fclose(sim_disk_fd);
    }

    // function that list all the File Descriptors and the disk content
    void listAll() {
        int i = 0;

        for (i = 0; i < MainDir.size(); i++) {
            cout << "index: " << i << ": FileName: " << MainDir[i]->getFileName() << " , isInUse: "
                 << MainDir[i]->isInUse() << endl;
        }

        char bufy;
        cout << "Disk content: '";
        for (i = 0; i < DISK_SIZE; i++) {
            cout << "(";
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = (int) fread(&bufy, 1, 1, sim_disk_fd);
            cout << bufy;
            cout << ")";
        }
        cout << "'" << endl;
    }

    // method that format the disk according to the block size given

    void fsFormat(int blockSize) {

        this->currentBlockSize = blockSize;
        this->maximumSize = currentBlockSize * currentBlockSize;
        this->amountOfFreeBlocks = DISK_SIZE / currentBlockSize;

        if (is_formated) {
            while (!MainDir.empty())
                DelFile(MainDir[0]->getFileName());
            delete[] OpenFileDescriptors;
            delete[] BitVector;
            MainDir.clear();
        }

        int amountOfBlocks = DISK_SIZE / currentBlockSize; // amount of blocks according to the block size

        BitVectorSize = amountOfBlocks; // bit vector size is according to the amount of blocks

        BitVector = new int[BitVectorSize]; // define the array of the vector

        OpenFileDescriptors = new FileDescriptor *[BitVectorSize];

        for (int i = 0; i < BitVectorSize; ++i) {
            BitVector[i] = 0;
            OpenFileDescriptors[i] = nullptr;
        }


        // redefine the values of the disk to be empty
        sim_disk_fd = fopen(DISK_SIM_FILE, "r+");
        assert(sim_disk_fd);

        for (int i = 0; i < DISK_SIZE; i++) {
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fwrite("\0", 1, 1, sim_disk_fd);
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
        // check that the file is printed

        is_formated = true; // indicate that the disk is formatted
        cout << "FORMAT DISK: number of blocks: " << amountOfBlocks;
    }

    // method that create file and return its FileDescriptor or -1 if the disk is empty

    int CreateFile(string fileName) {
        int currFdNum = -1;
        if (!is_formated) {
            return -1;
        } else {
            if (sizeCanUse > 0 && MainDir.size() < BitVectorSize) {
                // create fsFILE
                FsFile *tmpFsFile = new FsFile(currentBlockSize); // creating fsFile for the new file
                FileDescriptor *tmpFileDs = new FileDescriptor(fileName, tmpFsFile);
                // update MainDIr //update OpenFileDescriptor
                currFdNum = usableFD();
                if (currFdNum != -1) {
                    MainDir.emplace_back(tmpFileDs);
                    OpenFileDescriptors[currFdNum] = tmpFileDs;
                    return currFdNum; // return the FD number of the opened file
                }
            }
            return -1;
        }
    }

    int usableFD() {
        for (int i = 0; i < BitVectorSize; ++i) {
            if (OpenFileDescriptors[i] == nullptr) {
                return i;
            }
        }
        return -1;
    }


    // method that receive file name and open it if it close otherwise return -1
    int OpenFile(string fileName) {
        if (is_formated) {
            for (int i = 0; i < MainDir.size(); ++i) {
                if (MainDir[i]->getFileName() == fileName) {
                    if (MainDir[i]->isInUse()) {
                        return -1;
                    } else {
                        int currCheck = usableFD(); // getting a good fd to place the one we open
                        if (currCheck != -1) {
                            MainDir[i]->setInUse(true);
                            OpenFileDescriptors[currCheck] = MainDir[i];
                            return currCheck;
                        }

                    }
                }
            }
        }
        return -1;
    }

    // method that check if the fd sent is one of the fd that was open and close it. if it was close return -1 as string
    string CloseFile(int fd) {
        if (is_formated && OpenFileDescriptors[fd] != nullptr && fd != -1) {
            int locToChange = 0;
            string currFileName = OpenFileDescriptors[fd]->getFileName(); // get the File name using the fd
            OpenFileDescriptors[fd] = nullptr; // change the open file to null since we closing it
            MainDir[locToChange]->setInUse(false); // define the fd in the Main dir value of use to false
            locToChange = findFileAccordingToName(currFileName);
            MainDir[locToChange]->setInUse(false);
            return currFileName;
        }
        return "-1";
    }

    // Function that gets fd and string to write to the file. the function checks if the file exists and if there's enough space in the disk than write otherwise return -1
    int WriteToFile(int fd, char *buf, int len) {
        sim_disk_fd = fopen(DISK_SIM_FILE, "r+");

        if (is_formated && OpenFileDescriptors[fd] != nullptr && len <= maximumSize && len <= sizeCanUse) {
            int counter = 0;

            if (OpenFileDescriptors[fd]->getFsFile()->getIndexBlock() > -1) { // has been written to before
                if ((maximumSize - OpenFileDescriptors[fd]->getFsFile()->getFileSize()) >
                    0) { //has more space in the last block

                    int lastBlock = lastBlockUsed(fd);
                    int offSetInBlock = OpenFileDescriptors[fd]->getFsFile()->getFileSize() % currentBlockSize;
                    if (offSetInBlock != 0) { // there's block with usable space
                        for (int i = 0; i < currentBlockSize -
                                            offSetInBlock; ++i) { // for loop that go to the last block on the open spot found

                            if (fseek(sim_disk_fd, (lastBlock * currentBlockSize) + offSetInBlock + i, SEEK_SET) != 0) {
                                return -1;
                            }
                            if (fwrite(buf + i, 1, 1, sim_disk_fd) < 1) {
                                return -1;
                            }
                            len--;
                            counter++;
                            OpenFileDescriptors[fd]->getFsFile()->setFileSize(
                                    OpenFileDescriptors[fd]->getFsFile()->getFileSize() + 1);
                            sizeCanUse--;
                        }
                    }

                    for (int i = 0; i <= len / currentBlockSize +
                                         len % currentBlockSize; ++i) { // repeat the process if there's more len
                        if (OpenFileDescriptors[fd]->getFsFile()->getBlockInUse() < currentBlockSize &&
                            len > 0) { //the case of last block found before the last one available
                            int currIndex = lastBlock;
                            if ((OpenFileDescriptors[fd]->getFsFile()->getFileSize() %
                                 OpenFileDescriptors[fd]->getFsFile()->getBlockInUse()) == 0) {
                                currIndex = findNextFreeBlock();
                            }
                            int blockCounter = 4;
                            char currIndexChar = (char) currIndex;
                            int currIndexBlock = OpenFileDescriptors[fd]->getFsFile()->getIndexBlock();
                            currIndexChar += '0';
                            int freeSpotOnIndex = OpenFileDescriptors[fd]->getFsFile()->getBlockInUse();

                            if (fseek(sim_disk_fd, currIndexBlock * currentBlockSize + freeSpotOnIndex, SEEK_SET) !=
                                0) { // find the spot on the Index block to write the new allocated block
                                return -1;
                            }
                            if (fwrite(&currIndexChar, 1, 1, sim_disk_fd) <
                                1) { // write the block number to the index block
                                return -1;
                            }
                            OpenFileDescriptors[fd]->getFsFile()->setBlockInUse(
                                    OpenFileDescriptors[fd]->getFsFile()->getBlockInUse() + 1); //update the fsFile
                            if (fseek(sim_disk_fd, currIndex * currentBlockSize, SEEK_SET) !=
                                0) { // go to the new block location
                                return -1;
                            }
                            if (len < 4) {
                                blockCounter = len;
                            }
                            if (fwrite(buf + counter, 1, blockCounter, sim_disk_fd) < 1) {
                                return -1;
                            }
                            len -= blockCounter;
                            counter += blockCounter;
                            OpenFileDescriptors[fd]->getFsFile()->setFileSize(
                                    OpenFileDescriptors[fd]->getFsFile()->getFileSize() + blockCounter);
                            sizeCanUse -= blockCounter;
                        }
                    }


                } else {
                    return -1;
                }


            } else  // never written to
            {
                if (amountOfFreeBlocks >
                    (1 + (ceil(len / currentBlockSize)))) { // check if there's enough blocks to store the string
                    int currIndex = findNextFreeBlock();
                    OpenFileDescriptors[fd]->getFsFile()->setIndexBlock(currIndex); // update the index block in FsFile
                    int currentLen = len; //starting len
                    int currentUse = currentBlockSize; // current use that can be change
                    double ceiling = (double) len / currentBlockSize;
                    for (int i = 0; i < ceil(ceiling); ++i) { // writing to index the blocks allocated to it
                        fseek(sim_disk_fd, currIndex * currentBlockSize + i,
                              SEEK_SET); // finding the correct location of the index block
                        int currBlock = findNextFreeBlock();
                        char currBlockChar = (char) currBlock;
                        currBlockChar += 48;
                        // write the correct allocated blocks to the index block
                        if (fwrite(&currBlockChar, 1, 1, sim_disk_fd) < 1) {
                            return -1;
                        }
                        fseek(sim_disk_fd, currBlock * currentBlockSize,
                              SEEK_SET); // finding and writing the input of the string to the assigned block
                        if (currentLen / currentBlockSize < 1) {
                            currentUse = currentLen;
                        }
                        if (fwrite(buf + (i * currentBlockSize), 1, currentUse, sim_disk_fd) < 1) {
                            return -1;
                        }
                        OpenFileDescriptors[fd]->getFsFile()->setFileSize(
                                OpenFileDescriptors[fd]->getFsFile()->getFileSize() + currentUse);
                        currentLen -= currentBlockSize;
                    }
                    // update the different variables

                    OpenFileDescriptors[fd]->getFsFile()->setBlockInUse(
                            OpenFileDescriptors[fd]->getFsFile()->getBlockInUse() +
                            (ceil((double) len / currentBlockSize)));

                    amountOfFreeBlocks -= ceil((double) len / currentBlockSize);
                    sizeCanUse -= len;
                }
            }

        }
        return -1;
    }

    // method that return the index of the last free char place in index
    int lastBlockUsed(int fd) {
        int currentIndexBlock =
                OpenFileDescriptors[fd]->getFsFile()->getIndexBlock() * currentBlockSize; // first Index in index block
        int LastUseBlock = OpenFileDescriptors[fd]->getFsFile()->getBlockInUse(); // get the amount of offset we need to move to get to the last used block
        char lastBlockChar;
        if (fseek(sim_disk_fd, currentIndexBlock + LastUseBlock - 1, SEEK_SET) != 0) {
            return -1;
        }
        if (fread(&lastBlockChar, 1, 1, sim_disk_fd) < 1) {
            return -1;
        }
        int lastBlockIndex = lastBlockChar - '0';
        if (lastBlockIndex > 0) {
            return lastBlockIndex;
        }
        return -1;

    }

    int findNextFreeBlock() {
        for (int i = 0; i < BitVectorSize; i++) {
            if (BitVector[i] == 0) {
                BitVector[i] = 1; // Changing the block state to be in use
                return i;
            }
        }
        return -1;
    }


    //method that deletes the fileName and all of its data, including the FsFile
    int DelFile(string FileName) {
        if (is_formated) {
            int fd = findFileAccordingToName(FileName);
            if (fd != -1) {
                FileDescriptor *currFD = MainDir[fd];
                FsFile *currFSfile = currFD->getFsFile();
                int indexBlock = currFSfile->getIndexBlock();
                if (indexBlock > -1) {
                    if (DelRelatedDiskContent(currFSfile->getIndexBlock(), currFSfile) == 0) {
                        if (currFD->isInUse())
                            OpenFileDescriptors[fd] = nullptr;
                        BitVector[currFSfile->getIndexBlock()] = 0; // updating bitVector
                        sizeCanUse += currFSfile->getFileSize() +
                                      currentBlockSize; // updating the size according to the amount of freed memory
                    }
                }
                delete currFD->getFsFile();
                delete MainDir[fd];
                MainDir.erase(MainDir.begin() + fd);
                return 1;
            }
        }
        return -1;
    }

//Receives an index block number and deletes all of its related data from the disk
    int DelRelatedDiskContent(int indexBlock, FsFile *currFS) {
        char currBlock;
        int fileSize = currFS->getFileSize();
        int sizeToDelete = 0;
        if (currentBlockSize < fileSize) {
            sizeToDelete = currentBlockSize;
        } else {
            sizeToDelete = fileSize;
        }
        int amountOfBlocksToDel = currFS->getBlockInUse();
        for (int i = 0; i < amountOfBlocksToDel; i++) { // for loop that go over the blocks we need to delete
            int currentBlockINT = 0;
            fseek(sim_disk_fd, currentBlockSize * indexBlock + i,
                  SEEK_SET); // finding the correct location of the index block
            fread(&currBlock, 1, 1, sim_disk_fd);
            fseek(sim_disk_fd, currentBlockSize * indexBlock + i,
                  SEEK_SET); // finding the correct location according to the one we found on the index block
            fwrite("\0", 1, 1, sim_disk_fd);
            currentBlockINT = (int) currBlock - '0';
            fseek(sim_disk_fd, currentBlockSize * currentBlockINT, SEEK_SET);

            for (int j = 0; j < sizeToDelete; j++) {  // loop that rewrite the block we read with '\0'
                if (fwrite("\0", 1, 1, sim_disk_fd) < 1)
                    return -1;
            }
            BitVector[currentBlockINT] = 0; // updating bit vector according to the action I just did
            fileSize -= sizeToDelete;
            sizeToDelete = currentBlockSize; // making sure that we read block size or file size in case that file size smaller
            if (fileSize < currentBlockSize) {
                sizeToDelete = fileSize;
            }
        }
        return 0;
    }


//Returns the index of fileName in MainDir if exists and '-1' if not
    int findFileAccordingToName(string fileName) {
        for (int i = 0; i < (int) MainDir.size(); i++) {
            if (MainDir[i]->getFileName() == fileName)
                return i;
        }
        return -1;
    }

    // Method that read to buf len size from the fd
    int ReadFromFile(int fd, char *buf, int len) {
        buf[0] = '\0'; // method that Tamar told use once to avoid printing last value in failure
        FileDescriptor *currentFD = OpenFileDescriptors[fd];
        if (is_formated && currentFD != nullptr &&
            len <= currentFD->getFsFile()->getFileSize()) { // all the details for being able to read
            FsFile *currentFsFile = currentFD->getFsFile();
            int amountToRead;
            if (currentBlockSize < len) {
                amountToRead = currentBlockSize;
            } else {
                amountToRead = len;
            }
            int numOfBlocks = (len + currentBlockSize - 1) / currentBlockSize;
            char currentBlock;
            for (int i = 0; i < numOfBlocks; ++i) {
                fseek(sim_disk_fd, currentBlockSize * currentFsFile->getIndexBlock() + i,
                      SEEK_SET); // seeking the index block
                fread(&currentBlock, 1, 1, sim_disk_fd); //reading the index block
                int currentBlockInt = (int) currentBlock - '0';
                fseek(sim_disk_fd, currentBlockSize * currentBlockInt,
                      SEEK_SET); // seeking the current block according to the block number we read
                fread(buf, amountToRead, 1, sim_disk_fd); // reading the block we seek from to the buf
                buf = buf + amountToRead; // updating buf according to the amount of added values
                len = len - amountToRead; // updating len according to the amount of read values
            }
            strncpy(buf, "\0", 1); // define an \0 in last char of buf
            return len; // returning len in success
        }
        return -1;
    }


};

int main() {
    int blockSize;
    int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read;
    int _fd;

    fsDisk *fs = new fsDisk();
    int cmd_;
    while (1) {
        cin >> cmd_;
        switch (cmd_) {
            case 0:   // exit
                delete fs;
                exit(0);
                break;

            case 1:  // list-file
                fs->listAll();
                break;

            case 2:    // format
                cin >> blockSize;
                fs->fsFormat(blockSize);
                break;

            case 3:    // creat-file
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 4:  // open-file
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 5:  // close-file
                cin >> _fd;
                fileName = fs->CloseFile(_fd);
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 6:   // write-file
                cin >> _fd;
                cin >> str_to_write;
                fs->WriteToFile(_fd, str_to_write, strlen(str_to_write));
                break;

            case 7:    // read-file
                cin >> _fd;
                cin >> size_to_read;
                fs->ReadFromFile(_fd, str_to_read, size_to_read);
                cout << "ReadFromFile: " << str_to_read << endl;
                break;

            case 8:   // delete file
                cin >> fileName;
                _fd = fs->DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            default:
                break;
        }
    }

}