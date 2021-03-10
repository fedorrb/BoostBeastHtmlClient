#ifndef _SINGLETON
#define _SINGLETON
// ласс существующий в единственном экземл€ре
//дл€ хранени€ глобальных переменных
#include <string>
class Singleton;  // опережающее объ€вление
  
class SingletonDestroyer
{
  private:
    Singleton* p_instance;
  public:    
    ~SingletonDestroyer();
    void initialize( Singleton* p );
};
  
class Singleton
{
  private:
    static Singleton* p_instance;
    static SingletonDestroyer destroyer;
  protected: 
    Singleton() { }
    Singleton( const Singleton& );
    Singleton& operator=( Singleton& );
   ~Singleton() { }
    friend class SingletonDestroyer;
  public:
    static Singleton& getInstance();    
    //объ€влени€ глобальных переменных
    unsigned short int rayon;
    unsigned short int uzel;
    unsigned short int insCode;
    char insFio [51];
	std::string fileNameIn;
	std::string fileNameOut;
    std::string reqCode;
	std::string pathAdmin;
    std::string md5;
    unsigned long dwMajorVersion;
    unsigned long dwMinorVersion;
    unsigned long dwNumberOfProcessors;
    unsigned long dwPageSize;
    unsigned long dwRamSize;
    int screenX;
    int screenY;
    int screenMaxX;
    int screenMaxY;
	int vidpov;
    bool debug;
    std::string debugPath;
};
#endif