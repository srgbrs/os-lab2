//
//  main.cpp
//  os_lab2
//
//  Created by Sergey on 08.11.2020.
//

#include <iostream>
#include <ostream>

using namespace std;

struct pageInformation {
    bool isAvailable = true;
    int quantity;
    int8_t* topFreeBlock;
};

class myAlloc {
public:
    int8_t* ptr;
    pageInformation* pages = new pageInformation[12];
    
    myAlloc(){
        ptr = (int8_t*)malloc(12 * 1024);
        
        int8_t* page_ptr = ptr;
        for (int i=0; i<12; i++) {
            pageInformation info = pageInformation();
            info.isAvailable = true;
            info.quantity = 1023;
            info.topFreeBlock = page_ptr;
            
            pages[i] = info;
            page_ptr = page_ptr + 1024;
        }
    }
    
    void* allocation(size_t size) {
        if (size > 1024/2) {
            
            if (size + 5 >= 1024) {
                int beggin_page = page(size);
                int required_pages = ceil( size / 1024);
                int current_size = size;
                int8_t* result = pages[beggin_page].topFreeBlock;
                
                for (int i = beggin_page; i<beggin_page+required_pages+1; i++) {
                    pages[i].isAvailable = true;
                    pages[i].quantity = 0;
                    int8_t* cursor_ptr = pages[i].topFreeBlock;
                    
                    *cursor_ptr = false;
                    if (current_size >= 1024) {
                        current_size = current_size - 1024;
                        pages[i].topFreeBlock = pages[i].topFreeBlock + 1023;
                        makeHeader(cursor_ptr + 1, current_size);
                    } else {
                        pages[i].topFreeBlock = pages[i].topFreeBlock + (current_size + 5);
                        makeHeader(cursor_ptr + 1, current_size);
                    }
                }
            } else
            {
                for (int i=0; i<12; i++) {
                    if (pages[i].isAvailable) {
                        int8_t* cursor_ptr = pages[i].topFreeBlock;
                        int8_t* result_ptr = cursor_ptr;
                        *cursor_ptr = false;
                        makeHeader(cursor_ptr + 1, size);
                        
                        cursor_ptr = cursor_ptr + (1023);
                        pages[i].isAvailable = false;
                        pages[i].quantity = 0;
                        pages[i].topFreeBlock = cursor_ptr + size + 1024;
                        return (void*)result_ptr;
                    }
                }
            }
            return nullptr;
        }
        else {
            size_t requiredSize = size_calc(size);
            for (int i=0; i<12; i++){
                if (pages[i].isAvailable) {
                    int8_t* cursor_ptr = pages[i].topFreeBlock;
                    *cursor_ptr = false;
                    makeHeader(cursor_ptr + 1, size);
                    
                    pages[i].isAvailable = false;
                    pages[i].quantity = (1024 / requiredSize) - 1;
                    pages[i].topFreeBlock = pages[i].topFreeBlock + requiredSize;
                    return (void*)cursor_ptr;
                }
            }
            
            for (int i=0; i<12; i++){
                if (!pages[i].isAvailable) {
                    int8_t* firstBlock = ptr + (1024 * i);
                    size_t sizeOfBlock = size_calc(getSize(firstBlock+1));
                    
                    if (pages[i].quantity > 0 && sizeOfBlock == requiredSize) {
                        int8_t* cursor_ptr = pages[i].topFreeBlock;
                        *cursor_ptr = false;
                        makeHeader(cursor_ptr + 1, size);
                        
                        pages[i].quantity = pages[i].quantity - 1;
                        pages[i].topFreeBlock = pages[i].topFreeBlock + sizeOfBlock;
                        return (void*)cursor_ptr;
                    }
                }
            }
        }
        return nullptr;
    }
    
    void* reallocation(void* pointer, size_t size) {
        int8_t* found_ptr = (int8_t*)pointer;
        bool isAvailable = *found_ptr;
        size_t size_before = getSize(found_ptr+1);
        int page_number = page_found(found_ptr);
        
        if (pages[page_number].isAvailable || size == size_before)
            return nullptr;
        if (size_calc(size_before) == size_calc(size)) {
            makeHeader(found_ptr+1, size);
            return (void*)found_ptr;
        }
        else
        {
            free(pointer);
            return allocation(size);
        }
        
    }
    
    void free(void* pointer) {
        int8_t* found_ptr = (int8_t*)pointer;
        bool isAvailable = *found_ptr;
        size_t size = getSize(found_ptr+1);
        int page_number = page_found(found_ptr);
        
        if (size < 1024/2)
        {
            *found_ptr = true;
            pages[page_number].quantity = pages[page_number].quantity + 1;
            
            if (pages[page_number].quantity == 1024 /2) {
                *found_ptr = true;
                pages[page_number].quantity = 1024 / size_calc(size);
                pages[page_number].isAvailable = true;
            } else if(size < 1024) {
                *found_ptr = true;
                makeHeader(found_ptr+1, 0);
                pages[page_number].isAvailable = true;
                pages[page_number].topFreeBlock = found_ptr;
            } else
            {
                int current_number = page_number;
                int8_t* current_ptr = found_ptr;
                
                while (size == 1024) {
                    *current_ptr = true;
                    pages[current_number].isAvailable = true;
                    pages[current_number].topFreeBlock = current_ptr;
                    current_number = current_number + 1;
                    current_ptr = current_ptr + 1024;
                    size = getSize(current_ptr + 1);
                }
                
                if (!pages[current_number].isAvailable)
                {
                    *current_ptr = true;
                    pages[current_number].isAvailable = true;
                    pages[current_number].topFreeBlock = current_ptr;
                }
            }
        }
    }
    
    size_t getSize(int8_t* ptr){ //
        int space = 0;
        size_t size = 0;
        while (space < 4) {
            if ((size_t) * (ptr + space) == 255)
                size = size + 256;
            else {
                size = size + (size_t) * (ptr + space);
            }
            space++;
        }
        return size;
    }
    
    size_t size_calc(size_t s){
        size_t requiredSize = s + 5;
        size_t cursor = 16;
        while (cursor != 16) {
            if (cursor > requiredSize) {
                return cursor;
            } else {
                cursor = cursor*2;
            }
        }
        return cursor;
    }
    
    int page_found(int8_t* pointer)
    {
        int8_t* current_ptr = ptr;
        for (int z = 0; z<12; z++) {
            if (pointer >= current_ptr && pointer < current_ptr+1024) {
                return z;
            }
            current_ptr = current_ptr + 1024;
        }
        return -1;
    }
    
    void makeHeader (int8_t* ptr, size_t s) {
        int space = 0;
        int maxSizeCell = floor(s / 256);
        if (maxSizeCell == 4)
        {
            while (maxSizeCell < 0)
            {
                *(ptr+space) = 255;
                maxSizeCell == maxSizeCell - 1;
                space = space + 1;
            }
            return;
        }
        
        int leftover = s - (maxSizeCell * 256);
        while (space != 3 || maxSizeCell >0)
        {
            if (maxSizeCell ==0) {
                *(ptr + space) = 0;
            } else {
                *(ptr + space) = 255;
                maxSizeCell = maxSizeCell - 1;
            }
            space = space + 1;
        }
        *(ptr + space) = leftover;
    }
    
    int page(size_t s) {
        int required_page = ceil(s / 1024);
        int page;
        
        for (int i=0; i<12; i++){
            if (pages[i].isAvailable) {
                page = i;
                for (int z=i; z< 12; z++) {
                    if (z == 11)
                        return -1;
                    if (pages[z].isAvailable && pages[z+1].isAvailable){
                        required_page = required_page - 1;
                        if (required_page ==0)
                            return page;
                    }
                    if (!pages[z].isAvailable && pages[z].isAvailable) {
                        page = z;
                        required_page = ceil(s / 1024);
                    }
                }
            }
        }
        return 0;
    }
    
    void detailedInfo() {
        for (int i=0; i<12; i++)
        {
            cout << "Страница: " << i << "\n";
            if (!pages[i].isAvailable) {
                cout << "Not available";
                
                int8_t* cursor_ptr = ptr + (i*1024);
                cout << (void*)cursor_ptr;
                size_t sizeOfBlock = getSize(cursor_ptr);
                size_t current_size = size_calc(sizeOfBlock);
                
                int block = 1;
                if (current_size == 1024) {
                    cout << "block=page\n";
                    cout << " | BLock " << block << " |address " << (void*)cursor_ptr << " |available " << (bool)*cursor_ptr << " |data size " << sizeOfBlock << "\n";
                    continue;
                }
                
                while (cursor_ptr != pages[i].topFreeBlock) {
                    cout << "| BLock " << block <<  "| address " << (void*)cursor_ptr << "| available " << (bool)*cursor_ptr << "| data size " << getSize(cursor_ptr+1) << "\n";

                    block++;
                    if ((size_t) * (cursor_ptr + 5) > 1024)
                        break;
                    cursor_ptr = cursor_ptr + current_size;
                }
                
            } else {
                cout << "Available\n";
                cout << (void*)pages[i].topFreeBlock << "\n";
            }
        }
    }
};


int main(int argc, const char * argv[]) {

    myAlloc my = myAlloc();
    
    cout << my.allocation(30);
    cout << my.allocation(15);
    void* addr1 = my.allocation(10);
    cout << addr1;
    my.detailedInfo();
    my.free(addr1);
    my.detailedInfo();
    return 0;
}
