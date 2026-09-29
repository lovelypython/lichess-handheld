#include "chess_engine.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static int fileOf(int s){return s&7;}
static int rankOf(int s){return s>>3;}
static bool inside(int f,int r){return f>=0&&f<8&&r>=0&&r<8;}

int ChessBoard::squareFromName(char file,char rank){
  if(file<'a'||file>'h'||rank<'1'||rank>'8')return -1;
  return (file-'a')+(rank-'1')*8;
}
String ChessBoard::squareName(int sq){
  if(sq<0||sq>=64)return "--";
  String s; s+=char('a'+fileOf(sq)); s+=char('1'+rankOf(sq)); return s;
}

void ChessBoard::reset(){
  const char* start="RNBQKBNRPPPPPPPP                                pppppppprnbqkbnr";
  for(int i=0;i<64;i++)cells_[i]=start[i]==' '?0:start[i];
  whiteToMove_=true;castle_=0x0F;epSquare_=-1;halfmove_=0;fullmove_=1;
}

bool ChessBoard::loadFEN(const String& fen){
  for(char& c:cells_)c=0;
  int field=0,file=0,rank=7;String token;
  for(size_t i=0;i<=fen.length();i++){
    char c=i<fen.length()?fen[i]:' ';
    if(c==' '){
      if(field==0){if(rank!=0||file!=8)return false;}
      else if(field==1)whiteToMove_=(token=="w");
      else if(field==2){castle_=0;for(char x:token){if(x=='K')castle_|=1;if(x=='Q')castle_|=2;if(x=='k')castle_|=4;if(x=='q')castle_|=8;}}
      else if(field==3)epSquare_=token=="-"?-1:squareFromName(token[0],token[1]);
      else if(field==4)halfmove_=atoi(token.c_str());
      else if(field==5){fullmove_=max(1,atoi(token.c_str()));return true;}
      field++;token="";continue;
    }
    if(field==0){
      if(c=='/'){if(file!=8||rank<=0)return false;rank--;file=0;continue;}
      if(c>='1'&&c<='8'){file+=c-'0';if(file>8)return false;continue;}
      if(!strchr("PNBRQKpnbrqk",c)||file>=8)return false;
      cells_[rank*8+file++]=c;
    }else token+=c;
  }
  return field>=4;
}

void ChessBoard::add(ChessMove* out,int& n,int maxMoves,int from,int to,char promo,uint8_t flags){
  if(n<maxMoves)out[n++]={int8_t(from),int8_t(to),promo,flags};
}

bool ChessBoard::squareAttacked(int sq,bool byWhite) const{
  int f=fileOf(sq),r=rankOf(sq);
  int pr=r+(byWhite?-1:1);
  if(pr>=0&&pr<8)for(int df:{-1,1}){
    int pf=f+df;if(inside(pf,pr)&&cells_[pr*8+pf]==(byWhite?'P':'p'))return true;
  }
  static const int kx[8]={1,2,2,1,-1,-2,-2,-1};
  static const int ky[8]={2,1,-1,-2,-2,-1,1,2};
  for(int i=0;i<8;i++){int x=f+kx[i],y=r+ky[i];if(inside(x,y)&&cells_[y*8+x]==(byWhite?'N':'n'))return true;}
  static const int dx[8]={1,1,-1,-1,1,-1,0,0};
  static const int dy[8]={1,-1,1,-1,0,0,1,-1};
  for(int d=0;d<8;d++){
    int x=f+dx[d],y=r+dy[d];
    while(inside(x,y)){
      char p=cells_[y*8+x];
      if(p){
        if(isWhite(p)==byWhite){char t=typeOf(p);if(t=='Q'||(d<4&&t=='B')||(d>=4&&t=='R'))return true;}
        break;
      }
      x+=dx[d];y+=dy[d];
    }
  }
  for(int x=f-1;x<=f+1;x++)for(int y=r-1;y<=r+1;y++)if(inside(x,y)&&(x!=f||y!=r)&&cells_[y*8+x]==(byWhite?'K':'k'))return true;
  return false;
}

bool ChessBoard::inCheck(bool white) const{
  char king=white?'K':'k';int sq=-1;for(int i=0;i<64;i++)if(cells_[i]==king){sq=i;break;}
  return sq<0||squareAttacked(sq,!white);
}

int ChessBoard::generatePseudo(ChessMove* out,int maxMoves) const{
  int n=0;bool us=whiteToMove_;
  for(int from=0;from<64;from++){
    char p=cells_[from];if(!p||isWhite(p)!=us)continue;
    int f=fileOf(from),r=rankOf(from);char t=typeOf(p);
    if(t=='P'){
      int dir=us?1:-1,start=us?1:6,prom=us?7:0;
      int one=from+dir*8;
      if(one>=0&&one<64&&!cells_[one]){
        if(rankOf(one)==prom)for(char q:{'q','r','b','n'})add(out,n,maxMoves,from,one,q,MF_PROMOTION);
        else add(out,n,maxMoves,from,one);
        int two=from+dir*16;if(r==start&&!cells_[two])add(out,n,maxMoves,from,two);
      }
      for(int df:{-1,1}){
        int x=f+df,y=r+dir;if(!inside(x,y))continue;int to=y*8+x;
        if(cells_[to]&&isWhite(cells_[to])!=us){
          if(y==prom)for(char q:{'q','r','b','n'})add(out,n,maxMoves,from,to,q,MF_CAPTURE|MF_PROMOTION);
          else add(out,n,maxMoves,from,to,0,MF_CAPTURE);
        }else if(to==epSquare_)add(out,n,maxMoves,from,to,0,MF_CAPTURE|MF_EN_PASSANT);
      }
    }else if(t=='N'){
      static const int dx[8]={1,2,2,1,-1,-2,-2,-1},dy[8]={2,1,-1,-2,-2,-1,1,2};
      for(int i=0;i<8;i++){int x=f+dx[i],y=r+dy[i];if(!inside(x,y))continue;int to=y*8+x;char q=cells_[to];if(!q||isWhite(q)!=us)add(out,n,maxMoves,from,to,0,q?MF_CAPTURE:0);}
    }else if(t=='B'||t=='R'||t=='Q'){
      static const int dx[8]={1,1,-1,-1,1,-1,0,0},dy[8]={1,-1,1,-1,0,0,1,-1};
      int d0=t=='B'?0:(t=='R'?4:0),d1=t=='B'?4:8;
      for(int d=d0;d<d1;d++){int x=f+dx[d],y=r+dy[d];while(inside(x,y)){int to=y*8+x;char q=cells_[to];if(!q)add(out,n,maxMoves,from,to);else{if(isWhite(q)!=us)add(out,n,maxMoves,from,to,0,MF_CAPTURE);break;}x+=dx[d];y+=dy[d];}}
    }else if(t=='K'){
      for(int x=f-1;x<=f+1;x++)for(int y=r-1;y<=r+1;y++)if(inside(x,y)&&(x!=f||y!=r)){int to=y*8+x;char q=cells_[to];if(!q||isWhite(q)!=us)add(out,n,maxMoves,from,to,0,q?MF_CAPTURE:0);}
      if(us&&from==4&&!inCheck(true)){
        if((castle_&1)&&cells_[7]=='R'&&!cells_[5]&&!cells_[6]&&!squareAttacked(5,false)&&!squareAttacked(6,false))add(out,n,maxMoves,4,6,0,MF_CASTLE);
        if((castle_&2)&&cells_[0]=='R'&&!cells_[1]&&!cells_[2]&&!cells_[3]&&!squareAttacked(3,false)&&!squareAttacked(2,false))add(out,n,maxMoves,4,2,0,MF_CASTLE);
      }else if(!us&&from==60&&!inCheck(false)){
        if((castle_&4)&&cells_[63]=='r'&&!cells_[61]&&!cells_[62]&&!squareAttacked(61,true)&&!squareAttacked(62,true))add(out,n,maxMoves,60,62,0,MF_CASTLE);
        if((castle_&8)&&cells_[56]=='r'&&!cells_[57]&&!cells_[58]&&!cells_[59]&&!squareAttacked(59,true)&&!squareAttacked(58,true))add(out,n,maxMoves,60,58,0,MF_CASTLE);
      }
    }
  }
  return n;
}

void ChessBoard::applyUnchecked(const ChessMove& m){
  bool mover=whiteToMove_;char p=cells_[m.from],movedType=typeOf(p),captured=cells_[m.to];
  cells_[m.from]=0;
  if(m.flags&MF_EN_PASSANT)cells_[m.to+(mover?-8:8)]=0;
  if(m.flags&MF_CASTLE){
    if(m.to==6){cells_[5]=cells_[7];cells_[7]=0;}else if(m.to==2){cells_[3]=cells_[0];cells_[0]=0;}
    else if(m.to==62){cells_[61]=cells_[63];cells_[63]=0;}else if(m.to==58){cells_[59]=cells_[56];cells_[56]=0;}
  }
  if(m.promotion)p=mover?toupper(m.promotion):tolower(m.promotion);
  cells_[m.to]=p;
  if(movedType=='K'){if(mover)castle_&=~3;else castle_&=~12;}
  if(m.from==0||m.to==0)castle_&=~2;
  if(m.from==7||m.to==7)castle_&=~1;
  if(m.from==56||m.to==56)castle_&=~8;
  if(m.from==63||m.to==63)castle_&=~4;
  epSquare_=-1;
  if(movedType=='P'&&abs(m.to-m.from)==16)epSquare_=(m.from+m.to)/2;
  halfmove_=(movedType=='P'||captured)?0:halfmove_+1;
  if(!mover)fullmove_++;
  whiteToMove_=!whiteToMove_;
}

int ChessBoard::generateLegal(ChessMove* out,int maxMoves) const{
  ChessMove pseudo[256];int pn=generatePseudo(pseudo,256),n=0;bool mover=whiteToMove_;
  for(int i=0;i<pn;i++){
    ChessBoard b=*this;b.applyUnchecked(pseudo[i]);
    if(!b.inCheck(mover)&&n<maxMoves)out[n++]=pseudo[i];
  }
  return n;
}

bool ChessBoard::isLegal(const ChessMove& m) const{
  ChessMove a[256];int n=generateLegal(a,256);
  for(int i=0;i<n;i++)if(a[i].from==m.from&&a[i].to==m.to&&tolower(a[i].promotion)==tolower(m.promotion))return true;
  return false;
}

bool ChessBoard::findLegal(int from,int to,char promotion,ChessMove& out) const{
  ChessMove a[256];int n=generateLegal(a,256);char want=promotion?tolower(promotion):0;
  for(int i=0;i<n;i++)if(a[i].from==from&&a[i].to==to){
    if(!a[i].promotion||tolower(a[i].promotion)==(want?want:'q')){out=a[i];return true;}
  }
  return false;
}

bool ChessBoard::apply(const ChessMove& m){if(!isLegal(m))return false;applyUnchecked(m);return true;}
bool ChessBoard::applyUCI(const String& u,ChessMove* applied){
  if(u.length()<4)return false;
  int f=squareFromName(u[0],u[1]),t=squareFromName(u[2],u[3]);
  char p=u.length()>4?u[4]:0;ChessMove m;
  if(!findLegal(f,t,p,m))return false;
  applyUnchecked(m);
  if(applied)*applied=m;
  return true;
}
String ChessBoard::uci(const ChessMove& m) const{String s=squareName(m.from)+squareName(m.to);if(m.promotion)s+=char(tolower(m.promotion));return s;}

bool ChessBoard::parseSAN(const String& input,ChessMove& out) const{
  String s=input;
  while(s.length()&&strchr("+#!?",s[s.length()-1]))s.remove(s.length()-1);
  if(s=="O-O"||s=="0-0")return findLegal(whiteToMove_?4:60,whiteToMove_?6:62,0,out);
  if(s=="O-O-O"||s=="0-0-0")return findLegal(whiteToMove_?4:60,whiteToMove_?2:58,0,out);
  char promo=0;int eq=s.indexOf('=');if(eq>=0&&eq+1<int(s.length()))promo=tolower(s[eq+1]);
  int destPos=-1;
  for(int i=int(s.length())-2;i>=0;i--)if(s[i]>='a'&&s[i]<='h'&&s[i+1]>='1'&&s[i+1]<='8'){destPos=i;break;}
  if(destPos<0)return false;
  int to=squareFromName(s[destPos],s[destPos+1]);
  char type=(s[0]>='A'&&s[0]<='Z'&&strchr("KQRBN",s[0]))?s[0]:'P';
  int start=type=='P'?0:1;
  char fileHint=0,rankHint=0;
  for(int i=start;i<destPos;i++){char c=s[i];if(c>='a'&&c<='h')fileHint=c;if(c>='1'&&c<='8')rankHint=c;}
  ChessMove moves[256];int n=generateLegal(moves,256);
  for(int i=0;i<n;i++){
    ChessMove m=moves[i];char p=cells_[m.from];
    if(typeOf(p)!=type||m.to!=to)continue;
    if(promo&&tolower(m.promotion)!=promo)continue;
    if(!promo&&m.promotion&&tolower(m.promotion)!='q')continue;
    if(fileHint&&fileOf(m.from)!=fileHint-'a')continue;
    if(rankHint&&rankOf(m.from)!=rankHint-'1')continue;
    out=m;return true;
  }
  return false;
}

String ChessBoard::san(const ChessMove& m) const{
  if(!m.valid())return "?";
  char p=cells_[m.from],t=typeOf(p);String s;
  if((m.flags&MF_CASTLE)|| (t=='K'&&abs(m.to-m.from)==2))s=m.to>m.from?"O-O":"O-O-O";
  else{
    bool capture=(m.flags&MF_CAPTURE)||cells_[m.to]!=0;
    if(t!='P'){
      s+=t;bool sameFile=false,sameRank=false,ambiguous=false;ChessMove all[256];int n=generateLegal(all,256);
      for(int i=0;i<n;i++)if(all[i].from!=m.from&&all[i].to==m.to&&typeOf(cells_[all[i].from])==t){ambiguous=true;if(fileOf(all[i].from)==fileOf(m.from))sameFile=true;if(rankOf(all[i].from)==rankOf(m.from))sameRank=true;}
      if(ambiguous){if(!sameFile)s+=char('a'+fileOf(m.from));else if(!sameRank)s+=char('1'+rankOf(m.from));else{s+=char('a'+fileOf(m.from));s+=char('1'+rankOf(m.from));}}
    }else if(capture)s+=char('a'+fileOf(m.from));
    if(capture)s+='x';
    s+=squareName(m.to);
    if(m.promotion){s+='=';s+=char(toupper(m.promotion));}
  }
  ChessBoard b=*this;b.applyUnchecked(m);if(b.inCheck(b.whiteToMove_))s+=b.checkmate()?'#':'+';return s;
}

bool ChessBoard::checkmate() const{ChessMove m[1];return inCheck(whiteToMove_)&&generateLegal(m,1)==0;}
