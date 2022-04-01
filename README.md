# Scenariusz 5 - współbieżne serwery TCP
## 1. Cel

Jak pewnie większość z Państwa zauważyła serwer wykorzystujący do komunikacji protokół UDP może obsługiwać wielu klientów równocześnie, ponieważ nie nawiązuje z żadnym z nich połączenia i przetwarza wszystkie otrzymane komunikaty niezależnie od tego od kogo je otrzyma. Inaczej jest w przypadku serwera TCP, który nawiązuje połączenie z jednym klientem. Dopóki to połączenie trwa, serwer TCP nie obsługuje innych klientów. Celem tych zajęć jest stworzenie współbieżnego serwera TCP, czyli takiego który obsługuje wielu klientów jednocześnie.
## 2. Możliwe rozwiązania

Nasze zadanie możemy rozwiązać na kilka sposobów.

Wariant 1. Każdy klient może być obsługiwany przez oddzielny proces, który zostaje stworzony kiedy klient łączy się z serwerem i działa dopóki klient nie zakończy połączenia.

Wariant 2. Każdy klient może być obsługiwany przez oddzielny wątek, który zostaje stworzony kiedy klient łączy się z serwerem i działa dopóki klient nie zakończy połączenia.

Wariant 3. Klienci mogą być obsługiwani przez pulę procesów lub wątków o ustalonym rozmiarze.

Wariant 4. Klienci mogą być obsługiwani przez pojedynczy wątek, który utrzymuje wiele otwartych połączeń i obsługuje klientów w miarę napływu danych (takiemu rozwiązaniu będzie poświęcone następne laboratorium).

Przyjrzyjmy się teraz szczegółowo poszczególnym metodom. Nie będziemy skupiać się na objaśnianiu funkcji do obsługi procesów i wątków, ponieważ poznali je Państwo na przedmiocie Programowanie Współbieżne. Zobaczymy natomiast jak wykorzystać zdobytą wiedzę do implementacji współbieżnych serwerów sieciowych.
## 3. Osobne procesy

Schemat działania serwera tworzącego osobny proces dla każdego połączenia jest następujący:

    sock = socket(...);
    bind(sock, ...);
    listen(sock, ...);
    
    while(1) {
      fd = accept(sock, ...);
      switch (fork()) {
         case -1: // błąd
         case 0:  // potomek
           close(sock);
            // obsługuj klienta korzystając z gniazda fd
            // typowe jest wywołanie w tym miejscu funkcji exec
           exit(0);
         default: 
           close(fd);
            // proces główny nie może wykonać wait()
      }
    }

Przypomnienie informacji o procesach.

Podane rozwiązanie ma poważną wadę. Proces główny nie może wykonać blokującej funkcji wait(), więc po zakończeniu działania proces potomny staje się procesem zombie. Dlaczego jest to niepożądane? Wystarczy, że wyobrazimy sobie serwer, który ma wielu klientów i działa bardzo długo. W pewnym momencie utworzenie nowego procesu stanie się niemożliwe, ponieważ procesy zombie zajmą wszystkie dostępne struktury systemowe do obsługi procesów.

Co możemy w tej sytuacji zrobić? Jednym ze sposobów jest ignorowanie przez proces główny sygnałów informujących o zakończeniu potomka.

    struct sigaction action;
    sigset_t block_mask;

    sigemptyset (&block_mask);
    action.sa_handler = SIG_IGN;
    action.sa_mask = block_mask;
    action.sa_flags = 0;
  
    if (sigaction (SIGCHLD, &action, 0) == -1)  
      syserr("sigaction");

Taki sposób jest charakterystyczny dla systemów posiksowych i może nie być obsługiwany przez starsze i niestandardowe systemy. Wówczas rozwiązaniem może być wywoływanie funkcji wait() w obsłudze sygnału SIGCHLD.

Przypomnienie informacji o sygnałach.
Przykład

We wszystkich dzisiejszych przykładach klient jest taki sam i znajduje się w pliku client.c.

Serwer dla wariantu 1 znajduje się w pliku proc_server.c.
Ćwiczenia

    Przeanalizuj kod serwera i klienta.

    Wykonaj kilka testów.

    Popraw kod serwera w taki sposób, aby nie pojawiały się procesy zombie.

    Zmień serwera tak, aby zliczał, ile klientów łącznie się zgłosiło i wypisywał tę informację na standardowe wyjście w obsłudze sygnału SIGUSR1.

## 4. Osobne wątki

Wariant 2 jest podobny do wariantu 1. Zamiast nowego procesu po nawiązaniu połączenia tworzony jest nowy wątek. Nie ma w tym przypadku problemu analogicznego do problemu zombie, ponieważ wątki można utworzyć w trybie odłączonym (detached).

Przypomnienie informacji o wątkach.

Należy mieć na uwadze, że każdy wątek powinien mieć własne dane, więc przekazywane obiekty nie mogą być statyczne.
Ćwiczenia

    Przeanalizuj kod serwera wielowątkowego (plik thread_server.c)

    Wykonaj kilka testów.

    (*) Zmodyfikuj kod serwera, tak aby w sytuacji braku zasobów (pamięć, wątek) zamiast kończyć działanie, informował klienta o niemożności obsługi. Zadbaj, żeby nie doprowadzić do wycieku pamięci.

## 5. Pula procesów lub wątków

Wariant 3 może mieć wiele realizacji. W każdej z nich tworzymy na początku lub w miarę potrzeb ograniczony zbiór procesów lub wątków.

Najprostsze rozwiązanie pokazuje program z pliku preforked_server.c. Główne gniazdo nasłuchowe otwieramy w procesie głównym jeszcze przed utworzeniem puli procesów obsługi. Dzięki temu odziedziczą je po wywołaniu fork(). Każdy proces w pętli wywołuje (niezależnie od innych) accept() i wszystkie czekają na klientów.

Takie rozwiązanie może nie działać w przestarzałych systemach uniksowych, gdzie accept() nie jest wywołaniem systemowym, przez co nie jest niepodzielny. Zadziała w Linuksie, BSD (w tym MacOS), Solarisie, AIX i HP-UX i innych.

Nowsze systemy uniksowe obsługują niezależne otwieranie przez kilka procesów gniazda nasłuchującego na tym samym porcie. Żeby to zrobić, każdy z procesów musi ustawić opcję gniazda SO_REUSEPORT jeszcze przed wywołaniem bind(), i wszystkie procesy nasłuchujące na gnieździe muszą mieć ten sam uid. W tym podejściu procesy są całkowicie niezależne, mogą nawet być uruchamiane przez skrypt, który od razu zakończy działanie.

Przykładowy serwer znajduje się w pliku reuse_server.c, skrypty do uruchamiania i wyłączania w run_reuse_servers.sh i shutdown_reuse_servers.sh. Interesujące jest obejrzenie wyjścia programu ss (albo netstat) gdy jest uruchomionych kilka serwerów.

Dodatkową zaletą tego rozwiązania w systemie Linux jest bardziej sprawiedliwe budzenie procesów obsługujących połączenia w porównaniu z wywoływaniem accept() na tym samym gnieździe. Omijamy też problem nadmiernego obciążenia wątku/procesu odbierającego połączenia w kolejnym podejściu opisanym poniżej.

Następne rozwiązanie polega na wywołaniu accept() w procesie głównym i przekazywaniu otrzymanego gniazda wolnemu podwładnemu. Dla wątków jest to proste w implementacji, ponieważ wszystkie mają wspólną tablicę deskryptorów, więc wystarczy przekazać podwładnemu numer deskryptora poprzez struktury globalne, widziane przez wszystkie wątki (pamiętając oczywiście o synchronizacji w dostępie do tych struktur).

W przypadku puli procesów takie rozwiązanie jest dużo trudniejsze ze względu na to, że wspólne są tylko te deskryptory, które były otwarte podczas wykonywania fork(). Niemniej systemy uniksowe potrafią przekazywać deskryptory między procesami, używając gniazd z dziedziny uniksowej, lecz jest to raczej trikowe rozwiązanie.
## 6. Przekazywanie deskryptorów między procesami (*)

Deskryptory przekazuje się za pomocą funkcji sendmsg() i recvmsg(), które są najbardziej ogólnymi funkcjami przesyłania dla gniazd. Potrafią one:

    przekazać jednym wywołaniem kilka buforów (lub jak kto woli rozproszony bufor)

    przekazać dodatkową informację sterującą, w naszym przypadku deskryptor.

Przesyłając deskryptor, trzeba pamiętać, żeby równocześnie przesłać choć jeden znak/oktet, inaczej recvmsg() uzna to za koniec pliku.

Istnieje gotowa biblioteka libancillary, którą znajduje się w katalogu libancillary. Zainteresowanych zapraszamy na stronę www.normalesup.org/~george/comp/libancillary albo do rozdziału 15, pkt. 15.7 książki ,,UNIX® Network Programming Volume 1, Third Edition: The Sockets Networking API'' autorstwa W. Richarda Stevensa, Billa Fennera i Andrew M. Rudoffa. W rozdziałach 28 i 30 znajdziemy przykłady zastosowań.
Ćwiczenia

    Przeanalizuj fd_recv.c, fd_send.c i test.c.

    Wykonaj test i przekonaj się, że przekazywanie deskryptorów między procesami jest możliwe.

Ambitne ćwiczenia dla chętnych

    (*) Napisz prosty komunikator sieciowy, który czyta ze standardowego wejścia, a przeczytane dane zapisuje do gniazda i jednocześnie czyta z gniazda dane przychodzące z drugiej strony, a przeczytane dane wypisuje na standardowe wyjście.

    (*) Zmodyfikuj preforked_server.c tak, aby tylko proces główny wołał w pętli accept() i przekazywał otrzymane deskryptory podwładnym.

## Ćwiczenie punktowane (1.5 pkt)

Napisz programy do umieszczania plików na serwerze: file-server-tcp i file-client-tcp.

Klient przyjmuje trzy parametry - adres ip lub nazwę serwera, port serwera i nazwę pliku do przesłania. Klient łączy się z serwerem, przesyła mu nazwę pliku i jego rozmiar, a następnie przesyła plik i kończy działanie.

Serwer przyjmuje jeden parametr - numer portu, na którym nasłuchuje. Serwer działa w pętli nieskończonej. Dla każdego klienta tworzy osobny wątek. Wątek obsługujący klienta wypisuje na standardowe wyjście:

new client [adres_ip_klienta:port_klienta] size=[rozmiar_pliku] file=[nazwa_pliku] 

Następnie odczekuje 1 sekundę, odbiera plik, zapisuje go pod wskazaną nazwą i wypisuje na standardowe wyjście:

client [adres_ip_klienta:port_klienta] has sent its file of size=[rozmiar_pliku]
total size of uploaded files [suma_rozmiarów_odebranych_plików]

Po czym wątek kończy działanie.

Przetestuj swoje rozwiązanie uruchamiając klienta i serwera na różnych komputerach. Spróbuj jednocześnie przesłać do jednego serwera kilka dużych plików.

Rozwiązania można prezentować w trakcie zajęć nr 5 lub 6.
