#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

struct Person { std::string name; int id; };
struct Paar   { int e1, e2, kid; };

struct Stammbaum {
    std::vector<Person> personen;
    std::vector<Paar>   paare;
    int nid = 1;
    int add(const std::string& n){ personen.push_back({n,nid}); return nid++; }
    void link(int e1,int e2,int k){ paare.push_back({e1,e2,k}); }
    const Person* byId(int id) const {
        for(auto& p:personen) if(p.id==id) return &p; return nullptr;
    }
    int byName(const std::string& n) const {
        for(auto& p:personen) if(p.name==n) return p.id; return -1;
    }
    std::vector<std::pair<int,int>> parents(int kid) const {
        std::vector<std::pair<int,int>> r;
        for(auto& p:paare) if(p.kid==kid) r.push_back({p.e1,p.e2});
        return r;
    }
};

struct Grid {
    std::vector<std::string> rows;
    void ensure(int r, int c){
        while((int)rows.size()<=r) rows.push_back("");
        while((int)rows[r].size()<=c) rows[r]+=' ';
    }
    void put(int r, int c, const std::string& s){
        if(s.empty()) return;
        ensure(r, c+(int)s.size()-1);
        for(int i=0;i<(int)s.size();++i) rows[r][c+i]=s[i];
    }
    void putc(int r, int c, char ch){
        ensure(r,c);
        if(rows[r][c]==' ') rows[r][c]=ch;
    }
    void print(const std::string& ind="           ") const {
        for(auto& row:rows) std::cout<<ind<<row<<"\n";
    }
};

int calcDepth(const Stammbaum& sb, int id, std::vector<int>& v){
    if(std::find(v.begin(),v.end(),id)!=v.end()) return 0;
    v.push_back(id);
    auto ps=sb.parents(id);
    if(ps.empty()){ v.pop_back(); return 0; }
    int mx=0;
    for(auto& p:ps){
        mx=std::max(mx,calcDepth(sb,p.first,v));
        mx=std::max(mx,calcDepth(sb,p.second,v));
    }
    v.pop_back();
    return mx+1;
}

int calcMaxLen(const Stammbaum& sb, int id, std::vector<int>& v){
    if(std::find(v.begin(),v.end(),id)!=v.end()) return 0;
    v.push_back(id);
    auto* p=sb.byId(id);
    int mx=p?(int)p->name.size():0;
    for(auto& ep:sb.parents(id)){
        mx=std::max(mx,calcMaxLen(sb,ep.first,v));
        mx=std::max(mx,calcMaxLen(sb,ep.second,v));
    }
    v.pop_back();
    return mx;
}

std::string pad(const std::string& s, int w){
    return s + std::string(std::max(0, w-(int)s.size()), ' ');
}

// ============================================================
//  draw(id, row, gen, max_gen, W, vline, G, vis)
//
//  Schreibt die Vorfahren von id ins Grid ab Zeile row.
//  Der Name von id steht bereits im Grid (vom Aufrufer platziert).
//
//  Layout:
//    X(g) = (max_gen - g) * (W+5)   <- X-Position des Namens in Generation g
//    X_el  = X - (W+5)              <- X-Position der Elternspalte
//    X_cplus = X - 3                <- '+' im " -+- " Verbinder
//
//  Kerneigenschaft:
//    - Zeile row:   HauptName -+- (KindName schon da)
//    - Zeile row+1: NebenName -+   NUR wenn Neben-Elter KEINE eigenen Vorfahren hat
//                   sonst:   nur '!' fuer senkrechte Verbindungslinie
//
//  Hat der Neben-Elter eigene Vorfahren, erscheint sein Name NUR als
//  Kopf seines Neben-Blocks darunter (keine Dopplung in Zeile row+1).
// ============================================================

int draw(const Stammbaum& sb, int id,
         int row, int gen, int max_gen, int W, int vline,
         Grid& G, std::vector<int>& vis);

int draw(const Stammbaum& sb, int id,
         int row, int gen, int max_gen, int W, int vline,
         Grid& G, std::vector<int>& vis){

    if(std::find(vis.begin(),vis.end(),id)!=vis.end()) return row;
    vis.push_back(id);

    int start_row = row;
    auto ps = sb.parents(id);
    if(ps.empty()){ vis.pop_back(); return row; }

    int X      = (max_gen - gen) * (W + 5);
    int X_el   = X - (W + 5);
    int X_cplus= X - 3;   // '+' im " -+- " Verbinder

    // Elternpaare aufteilen
    struct PI { int h, n; bool h_hv, n_hv; };
    std::vector<PI> pis;
    for(auto& e : ps){
        // elter1 (e.first) immer in Zeile 0, elter2 (e.second) in Zeile 1 oder Neben-Block
        int a=e.first, b=e.second;
        bool ha=!sb.parents(a).empty() && std::find(vis.begin(),vis.end(),a)==vis.end();
        bool hb=!sb.parents(b).empty() && std::find(vis.begin(),vis.end(),b)==vis.end();
        pis.push_back({a, b, ha, hb});
    }

    // Neben-Eltern MIT Vorfahren -> eigene Bloecke darunter (kein Name in Zeile row+1)
    std::vector<int> neben;
    for(auto& pi : pis)
        if(pi.n_hv
           && std::find(vis.begin(),vis.end(),pi.n)==vis.end()
           && std::find(neben.begin(),neben.end(),pi.n)==neben.end())
            neben.push_back(pi.n);

    bool has_neben    = !neben.empty();

    // --- Erstes Elternpaar ---
    {
        auto& pi  = pis[0];
        auto* hp  = sb.byId(pi.h);
        auto* np  = sb.byId(pi.n);
        std::string hn = hp?hp->name:"?";
        std::string nn = np?np->name:"?";

        // Zeile row: HauptName -+- (KindName schon da)
        G.put(row, X_el, pad(hn,W) + " -+- ");

        // Zeile row+1:
        //   Neben-Elter hat KEINE Vorfahren -> Name schreiben: "NebenName -+"
        //   Neben-Elter hat Vorfahren       -> nur leer (sein Name kommt im Neben-Block)
        if(!pi.n_hv){
            G.put(row+1, X_el, pad(nn,W) + " -+");
        }
        // Externe vline in row+1 immer weiterführen
        if(vline >= 0) G.putc(row+1, vline, '!');

        // Haupt-Elter rekursiv
        int next_row = pi.n_hv ? row : row + 2;  // Zeile wenn Neben-Elter Vorfahren hat
        if(pi.h_hv)
            next_row = std::max(next_row, draw(sb, pi.h, row, gen+1, max_gen, W, -1, G, vis));

        row = next_row;
    }

    // --- Weitere Elternpaare ---
    for(int i=1;i<(int)pis.size();++i){
        auto& pi  = pis[i];
        auto* hp  = sb.byId(pi.h);
        auto* np  = sb.byId(pi.n);
        std::string hn = hp?hp->name:"?";
        std::string nn = np?np->name:"?";

        // Trennzeile
        G.putc(row, X_cplus, '!');
        if(vline >= 0) G.putc(row, vline, '!');
        row++;

        // Haupt -+
        G.put(row, X_el, pad(hn,W) + " -+");
        if(vline >= 0) G.putc(row, vline, '!');
        int next_row = row + 1;
        if(pi.h_hv)
            next_row = std::max(next_row, draw(sb, pi.h, row, gen+1, max_gen, W, -1, G, vis));
        row = next_row;

        // Neben -+ (nur wenn keine Vorfahren)
        if(!pi.n_hv){
            G.put(row, X_el, pad(nn,W) + " -+");
            if(vline >= 0) G.putc(row, vline, '!');
            row++;
        }
    }

    // --- Neben-Bloecke darunter ---
    for(int k=0;k<(int)neben.size();++k){
        int nid = neben[k];
        if(std::find(vis.begin(),vis.end(),nid)!=vis.end()) continue;

        // '!'-Linie von start_row+1 bis row
        for(int r=start_row+1; r<row; ++r){
            G.putc(r, X_cplus, '!');
            if(vline >= 0) G.putc(r, vline, '!');
        }
        G.putc(row, X_cplus, '!');
        if(vline >= 0) G.putc(row, vline, '!');
        row++;

        // Neben-Namen als Kopf seines Blocks schreiben
        auto* np2   = sb.byId(nid);
        std::string nname = np2?np2->name:"?";
        int X_nid   = (max_gen - (gen+1)) * (W+5);
        G.put(row, X_nid, nname);
        // "-+" nach dem Namen: zeigt Verbindung zur senkrechten Linie oben
        // Padding + "-+" so dass '+' bei X_cplus landet
        if(X_cplus - 1 > X_nid + (int)nname.size()){
            int pad_len = X_cplus - 1 - X_nid - (int)nname.size();
            G.put(row, X_nid + (int)nname.size(), std::string(pad_len, ' ') + "-+");
        }

        bool last     = (k==(int)neben.size()-1);
        int new_vline = (!last || vline>=0) ? X_cplus : -1;
        row = draw(sb, nid, row, gen+1, max_gen, W, new_vline, G, vis);
    }

    // Externe vline in allen unseren Zeilen fortführen
    if(vline >= 0){
        for(int r=start_row+1; r<row; ++r)
            G.putc(r, vline, '!');
    }

    vis.pop_back();
    return row;
}

void show(const Stammbaum& sb, const std::string& name){
    int id = sb.byName(name);
    if(id==-1){
        std::cout<<"Person \""<<name<<"\" nicht gefunden.\nVerfuegbare Personen:\n";
        for(auto& p:sb.personen) std::cout<<"  - "<<p.name<<"\n";
        return;
    }
    if(sb.parents(id).empty()){
        std::cout<<"Fuer \""<<name<<"\" sind keine Vorfahren bekannt.\n";
        return;
    }
    std::cout<<"Stammbaum der Vorfahren von "<<name<<":\n\n";

    std::vector<int> v1,v2;
    int max_gen = calcDepth(sb,id,v1);
    int W       = calcMaxLen(sb,id,v2);

    auto* pp = sb.byId(id);
    int X = max_gen*(W+5);
    Grid G;
    G.put(0, X, pp?pp->name:"?");

    std::vector<int> vis;
    draw(sb, id, 0, 0, max_gen, W, -1, G, vis);

    G.print();
    std::cout<<"\n";
}

void addPerson(Stammbaum& sb){
    std::string kn,e1n,e2n;
    std::cout<<"Name des neuen Kindes:  "; std::cin>>kn;
    std::cout<<"Name von Elternteil 1:  "; std::cin>>e1n;
    std::cout<<"Name von Elternteil 2:  "; std::cin>>e2n;
    int id1=sb.byName(e1n); if(id1==-1){id1=sb.add(e1n);std::cout<<"  -> "<<e1n<<" neu.\n";}
    int id2=sb.byName(e2n); if(id2==-1){id2=sb.add(e2n);std::cout<<"  -> "<<e2n<<" neu.\n";}
    int kid=sb.byName(kn);  if(kid==-1) kid=sb.add(kn);
    sb.link(id1,id2,kid);
    std::cout<<"  -> "<<kn<<" als Kind von "<<e1n<<" + "<<e2n<<" eingetragen.\n\n";
}

Stammbaum load(){
    Stammbaum sb;
    const std::string t[]={
        "David","Maria","Florian","Tanja","Anne","Jakob","Adam","Tanja","David",
        "Klaus","Anke","Gustav","Doris","Susanne","Bernd","Eva","Doris","Klaus",
        "Kai","Eva","Adam","Birgit","Maria","Florian","Ralf","Maria","Florian",
        "Nina","Anne","Jakob","Tim","Anne","Jakob","Sophie","Nina","Ralf",
        "Mia","Anke","Bernd","Pia","Anke","Bernd","Moritz","Susanne","Gustav",
        "Finn","Susanne","Gustav","Max","Pia","Finn","Sarah","Sophie","Max",
        "Gustav","Leonie","Fabian"
    };
    int a=sizeof(t)/sizeof(t[0]);
    for(int i=0;i+2<a;i+=3){
        int kid=sb.byName(t[i]);  if(kid==-1) kid=sb.add(t[i]);
        int e1=sb.byName(t[i+1]); if(e1==-1)  e1=sb.add(t[i+1]);
        int e2=sb.byName(t[i+2]); if(e2==-1)  e2=sb.add(t[i+2]);
        sb.link(e1,e2,kid);
    }
    return sb;
}

int main(){
    std::cout<<"=== Familien-Stammbaum ===\n\n";
    Stammbaum sb=load();
    std::string gesucht;
    std::cout<<"Vorfahren von wem anzeigen? Name: ";
    std::cin>>gesucht; std::cout<<"\n";
    int wahl=0;
    while(true){
        std::cout<<"------------------------------\n";
        std::cout<<"1) Stammbaum anzeigen\n";
        std::cout<<"2) Andere Person waehlen\n";
        std::cout<<"3) Kind hinzufuegen\n";
        std::cout<<"4) Beenden\n";
        std::cout<<"Wahl: "; std::cin>>wahl; std::cout<<"\n";
        if     (wahl==1) show(sb,gesucht);
        else if(wahl==2){std::cout<<"Neuer Name: ";std::cin>>gesucht;std::cout<<"\n";}
        else if(wahl==3) addPerson(sb);
        else{std::cout<<"Tschuess!\n";break;}
    }
    return 0;
}