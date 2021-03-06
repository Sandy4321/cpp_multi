#include <iostream>
#include <queue>
#include <string> 
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <sstream> 
#include <stdio.h>
#include <cstdlib>
 
using namespace std;

class Sort{
	public:
	Sort(size_t file_s, size_t file_num_s, size_t mem_s){//передаём длину файла, размер чтения за раз, чилсо
		file_size = file_s                                              //открываемых на чтение файлов и размер памяти
		file_num_size = file_num_s
		mem_size = mem_s
		//batch_size = buf_len/(sort_number + 1);
	}
	
	~Sort(){
		del();
	};

	void del(){
		//по окончании работы удаляем временные файлы
		system("rm -rf tmp");
		cout << "tmp folder removed.\n";
	}

	void init(const char * INPUT_FILE){
		// Разбиваем наш большой файл на множество отсортированных размером >= buf_len
		system("mkdir tmp");

		ifstream FileIn(INPUT_FILE, ios:: in | ios::binary);				
		size_t num = 0;//номер текущего файла вывода
		size_t position = 0;//смещение в исходном файле

		while (file_size > position){//пока не кончится файл
			FileIn.seekg(position);

			// количество байт, которые мы запишем в новый файл
			size_t l = mem_size;
			if(mem_size > file_size - position)//если до конца меньше чем объявленный буфер
				l = file_size - position;

			// считываем батч
			vector<int> buf(l / sizeof(int));
			FileIn.read(reinterpret_cast<char*>(buf.data()), buf.size()*sizeof(int));//считываем числа в буфер

			//сортируем
			sort(buf.begin(), buf.end());//сортируем буфер

			//выводим в новый файл				
			stringstream  touch_file;
			touch_file <<  "tmp/init" << num;
			ofstream FileOut;
			FileOut.open(touch_file.str(), ios::out | ofstream::binary);
			FileOut.write(reinterpret_cast<const char*>(&buf[0]), buf.size() * sizeof(int));//пишем в файд
			FileOut.close();//закрываем
			position += l;//сдвигаемся на размер буфера
			num++;

			//добавляем новый файл в очередь
			FILES.push(touch_file.str());
		}
		FileIn.close();
	}

	void file_sort(){
		// сортировка множества файлов
		int num = 0; // номер нового файла после merge
		while (FILES.size() > 1){

			cout << "FILES LEFT: "<< FILES.size() << "\n";
			vector<string> files; // батч файлов для merge

			for (int i=0; (i<file_num_size) && (i <= FILES.size()) ; ++i){//в этом цикле выбираем из FILES столько файлов, сколько можем открыть на чтение
				files.push_back(FILES.front());
				FILES.pop();
			}

			new_file = merge(files, num);
			files.clear();
			
			stringstream  touch_file;
			touch_file <<  "tmp/new" << num;
			FILES.push(touch_file.str());
			num++;
		}

		// по окончании скопируем оставшийся "собранный" файл в качестве result
		stringstream  cmd;
		cmd <<  "cp " << FILES.front() << " result";
		const string tmp = cmd.str();
		const char* cstr = tmp.c_str();
		cout << cstr << "\n";
		system(cstr);
		
	}

	void merge(vector<string> files, int k){

			vector<vector<int> > buf(files.size()); // буфер
			vector<int> len(files.size()); // оставшиеся длины файлов для чтения
			vector<int> position(files.size()); // позиция с которой читать
			vector<int> sorted(0); // отсортированный вектор вывода
			vector<bool> check(files.size()); // проверка на то, что  считываемый файл закончился
			vector<size_t> place(files.size()); // текущее место в буфере для каждого файла


			// считываем batсh из каждого файла и определяем начальные значения
			for (int i=0; i<buf.size(); ++i){
				ifstream FileIn(files[i], ios::in | ios::binary);
				FileIn.seekg(0, FileIn.end);
				len[i] = FileIn.tellg(); 
				FileIn.seekg(0);

				position[i] = min(mem_size, int(len[i]/sizeof(int)));
				cout << files[i] << " " << len[i] << "\n";
				len[i] -= position[i] * sizeof(int);
 				buf[i].resize(position[i]);			

				FileIn.read(reinterpret_cast<char*>(buf[i].data()), buf[i].size() * sizeof(int)); 
				FileIn.close();

 				place[i] = 0;
				check[i] = true;
			}

			bool full_check = true;	// проверка на то, что все файлы закончились			

			
			while (full_check){
				int minimum = INT_MAX;
				size_t min_pos = -1;

				// определяем минимальный элемент в текущей "строке"
				for (int i=0; i<buf.size(); i++){
					if ((check[i] == true) && (minimum >= buf[i][place[i]])){
						minimum = buf[i][place[i]];
						min_pos = i;
					}
				}
				
				// добавили значение в отсортированнын
				sorted.push_back(buf[min_pos][place[min_pos]]);
				place[min_pos] += 1;

				// если файл считанный буфер закончился
				if (place[min_pos] >= buf[min_pos].size()){
					
					// если файл считали полностью
					if (len[min_pos] < 1) {						
						check[min_pos] = false;
					}
					else{

					place[min_pos] = 0;

					ifstream FileIn(files[min_pos], ios::in | ios::binary);
					FileIn.seekg(position[min_pos] * sizeof(int));

					size_t pos = min(batch_size, int(len[min_pos]/sizeof(int)));
					len[min_pos] -= pos * sizeof(int);
					position[min_pos] += pos;

					buf[min_pos].clear();
					buf[min_pos].resize(pos);
					
					FileIn.read(reinterpret_cast<char*>(buf[min_pos].data()), buf[min_pos].size() * sizeof(int));

					}
				}

				// если буфер вывода заполнился
				if (sorted.size() >= batch_size / sizeof(int)){
					stringstream  adr;
					adr <<  "tmp/new" << k;
					ofstream FileOut(adr.str(), ios::out | ios::app | ios::binary);
					FileOut.write(reinterpret_cast<const char*>(&sorted[0]), sorted.size() * sizeof(int));

					sorted.clear();
					sorted.resize(0);

				}

				// проверяем, что есть ещё, что обрабатывать
				full_check = false;
				for (int i=0;i<check.size();i++)
					full_check += check[i];
			}

			
			// выведем, если ещё что-то осталось в отсортированном буфере
			stringstream  adr;
			adr <<  "tmp/new" << k;

			ofstream FileOut(adr.str(), ios::out | ios::app | ios::binary);
			FileOut.write(reinterpret_cast<const char*>(&sorted[0]), sorted.size() * sizeof(int));

			sorted.clear();

		}

	private:
		int buf_len; // размер буфера
		int sort_number; // количество файлов при сортировке
		int batch_size; // размер сортировочного батча
		queue<string> FILES; // очередь файлов
};


int main()
{
	size_t KB = 1024 ;
	//в качестве параметров возьмём объём файла, объём считывания данных за раз с диска и максимальное число поддерживаемых файлов на открытие.
	//И ещё количество оперативной памяти. С этими параметрами будем создавато класс Sort
	size_t mem_size = (size_t) atoll(512) * KB;
	size_t file_num_size_files = (size_t) atoll(2);

	char INPUT_FILE[] = "to_sort"
	ifstream FileIn(INPUT_FILE, ios:: in | ios::binary);				
	FileIn.seekg(0, FileIn.end);
	size_t file_size = FileIn.tellg();
	FileIn.close();//<---

	Sort s(file_size, file_num_size, mem_size);
	s.init(INPUT_FILE);
	s.file_sort();

    return 0;
}