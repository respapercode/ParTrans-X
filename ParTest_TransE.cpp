#include<iostream>
#include<cstring>
#include<cstdio>
#include<map>
#include<vector>
#include<string>
#include<ctime>
#include<algorithm>
#include<cmath>
#include<cstdlib>
#include<sstream>
#include<omp.h>
using namespace std;

bool debug=false;
bool L1_flag=1;

string version;

string dataset="";
string resultpath="";
int nthread = 1;
int n= 100;

map<string,int> relation2id,entity2id;
map<int,string> id2entity,id2relation;
map<string,string> mid2name,mid2type;
map<int,map<int,int> > entity2num;
map<int,int> e2num;
map<pair<string,string>,map<string,double> > rel_left,rel_right;
map<pair<int,int>, map<int,int> > ok;

int relation_num,entity_num;

string Int_to_String(int n)
{
    ostringstream stream;
    stream<<n;  //n为int类型
    return stream.str();
}
int String_to_Int(string s)
{
    stringstream ss;
    ss<<s;
    int num;
    ss>>num;
    return num;
}
double String_to_Double(string s)
{
    stringstream ss;
    ss<<s;
    double num;
    ss>>num;
    return num;
}

double sigmod(double x)
{
    return 1.0/(1+exp(-x));
}


double vec_len(vector<double> &a)
{
    double res=0;
    for (int i=0; i<a.size(); i++)
        res+=a[i]*a[i];
    res = sqrt(res);
    return res;
}
void vec_output(vector<double> a)
{
	for (int i=0; i<a.size(); i++)
	{
		cout<<a[i]<<"\t";
		if (i%10==9)
			cout<<endl;
	}
	cout<<"-------------------------"<<endl;
}

double sqr(double x)
{
    return x*x;
}

char buf[100000],buf1[100000];

int my_cmp(pair<double,int> a,pair<double,int> b)
{
    return a.first>b.first;
}

double cmp(pair<int,double> a, pair<int,double> b)
{
	return a.second<b.second;
}

class Test{
    vector<vector<double> > relation_vec,entity_vec;


    vector<int> h,l,r;
    vector<int> fb_h,fb_l,fb_r;
    
    double res ;
public:
    void add(int x,int y,int z, bool flag)
    {
    	if (flag)
    	{
        	fb_h.push_back(x);
        	fb_r.push_back(z);
        	fb_l.push_back(y);
        }
        ok[make_pair(x,z)][y]=1;
    }

    int rand_max(int x)
    {
        int res = (rand()*rand())%x;
        if (res<0)
            res+=x;
        return res;
    }
    double len;
    double calc_sum(int e1,int e2,int rel)
    {
        double sum=0;
        if (L1_flag)
        	for (int ii=0; ii<n; ii++)
            sum+=-fabs(entity_vec[e2][ii]-entity_vec[e1][ii]-relation_vec[rel][ii]);
        else
        for (int ii=0; ii<n; ii++)
            sum+=-sqr(entity_vec[e2][ii]-entity_vec[e1][ii]-relation_vec[rel][ii]);
        return sum;
    }
    void run()
    {
        
        FILE* f1 = fopen((resultpath+"/relation2vec."+version).c_str(),"r");
        FILE* f3 = fopen((resultpath+"/entity2vec."+version).c_str(),"r");

        cout<<relation_num<<' '<<entity_num<<endl;
        if(!f1)
        {
            printf(("can't open "+resultpath+"/relation2vec."+version).c_str());
            exit(-1);
        }
        if(!f3)
        {
            printf(("can't open "+resultpath+"/entity2vec."+version).c_str());
            exit(-1);
        }

        int relation_num_fb=relation_num;
        relation_vec.resize(relation_num_fb);
        for (int i=0; i<relation_num_fb;i++)
        {
            relation_vec[i].resize(n);
            for (int ii=0; ii<n; ii++)
                fscanf(f1,"%lf",&relation_vec[i][ii]);
        }
        entity_vec.resize(entity_num);
        cout<<"k=: "<<n<<endl;
        for (int i=0; i<entity_num;i++)
        {
            entity_vec[i].resize(n);
            for (int ii=0; ii<n; ii++)
                fscanf(f3,"%lf",&entity_vec[i][ii]);

            if (vec_len(entity_vec[i])-1>1e-3)
            	cout<<"wrong_entity"<<i<<' '<<vec_len(entity_vec[i])<<endl;
        }
        fclose(f1);
        fclose(f3);
		double lsum=0 ,lsum_filter= 0;
		double rsum = 0,rsum_filter=0;
		double lp_n=0,lp_n_filter;
		double rp_n=0,rp_n_filter;
		map<int,double> lsum_r,lsum_filter_r;
		map<int,double> rsum_r,rsum_filter_r;
		map<int,double> lp_n_r,lp_n_filter_r;
		map<int,double> rp_n_r,rp_n_filter_r;
		map<int,int> rel_num;

        #pragma omp parallel for firstprivate(ok) reduction(+:lsum,rsum,lp_n,rp_n,lsum_filter,rsum_filter,lp_n_filter,rp_n_filter)
        for (int testid = 0; testid<fb_l.size(); testid+=1)
		{
            time_t start,stop;
            start = time(NULL);
            
			int h = fb_h[testid];
			int l = fb_l[testid];
			int rel = fb_r[testid];
			double tmp = calc_sum(h,l,rel);
			// rel_num[rel]+=1;
			vector<pair<int,double> > a;
			for (int i=0; i<entity_num; i++)
			{
				double sum = calc_sum(i,l,rel);
				a.push_back(make_pair(i,sum));
			}
			sort(a.begin(),a.end(),cmp);
			double ttt=0;
			int filter = 0;
			for (int i=a.size()-1; i>=0; i--)
			{
				
                if(ok.find(make_pair(a[i].first,rel))!=ok.end() && ok[make_pair(a[i].first,rel)].count(l)>0)
                    ttt++;
                if(!(ok.find(make_pair(a[i].first,rel))!=ok.end() && ok[make_pair(a[i].first,rel)].count(l)>0))
                    filter+=1;        

				if (a[i].first ==h)
				{
					lsum+=a.size()-i;
					lsum_filter+=filter+1;
					// lsum_r[rel]+=a.size()-i;
					// lsum_filter_r[rel]+=filter+1;
					if (a.size()-i<=10)
					{
						lp_n+=1;
						// lp_n_r[rel]+=1;
					}
					if (filter<10)
					{
						lp_n_filter+=1;
						// lp_n_filter_r[rel]+=1;
					}
					break;
				}
			}
			a.clear();
			for (int i=0; i<entity_num; i++)
			{
				double sum = calc_sum(h,i,rel);
				a.push_back(make_pair(i,sum));
			}
			sort(a.begin(),a.end(),cmp);
			ttt=0;
			filter=0;
			for (int i=a.size()-1; i>=0; i--)
			{
				
                if(ok.find(make_pair(h,rel))!=ok.end() && ok[make_pair(h,rel)].count(a[i].first)>0)
                    ttt++;
                if(!(ok.find(make_pair(h,rel))!=ok.end() && ok[make_pair(h,rel)].count(a[i].first)>0))
                    filter+=1;

				if (a[i].first==l)
				{
					rsum+=a.size()-i;
					rsum_filter+=filter+1;
					// rsum_r[rel]+=a.size()-i;
					// rsum_filter_r[rel]+=filter+1;
					if (a.size()-i<=10)
					{
						rp_n+=1;
						// rp_n_r[rel]+=1;
					}
					if (filter<10)
					{
						rp_n_filter+=1;
						// rp_n_filter_r[rel]+=1;
					}
					break;
				}
			}
            stop = time(NULL);
            // cout<<"testid:"<<testid<<" time:"<<(stop-start)<<endl;
        }
        
		cout<<"left:"<<lsum/fb_l.size()<<'\t'<<lp_n/fb_l.size()<<"\t"<<lsum_filter/fb_l.size()<<'\t'<<lp_n_filter/fb_l.size()<<endl;
		cout<<"right:"<<rsum/fb_r.size()<<'\t'<<rp_n/fb_r.size()<<'\t'<<rsum_filter/fb_r.size()<<'\t'<<rp_n_filter/fb_r.size()<<endl;
        FILE* f = fopen("./all.TransE.log","a");
        fprintf(f, "left:%f\t%f\t%f\t%f\n",lsum/fb_l.size(),lp_n/fb_l.size(), lsum_filter/fb_l.size(),lp_n_filter/fb_l.size());
        fprintf(f, "right:%f\t%f\t%f\t%f\n\n",rsum/fb_r.size(),rp_n/fb_r.size(), rsum_filter/fb_r.size(),rp_n_filter/fb_r.size());
        fclose(f);
    }

};
Test test;

void prepare()
{
    FILE* f1 = fopen(("./"+dataset+"/entity2id.txt").c_str(),"r");
	FILE* f2 = fopen(("./"+dataset+"/relation2id.txt").c_str(),"r");
	int x;
    if(!f1)
    {
        printf(("can't open ./"+dataset+"/entity2id.txt").c_str());
        exit(-1);
    }
    if(!f2)
    {
        printf(("can't open ./"+dataset+"/relation2id.txt").c_str());
        exit(-1);
    }

	while (fscanf(f1,"%s%d",buf,&x)==2)
	{
		string st=buf;
		entity2id[st]=x;
		id2entity[x]=st;
		mid2type[st]="None";
		entity_num++;
	}
	while (fscanf(f2,"%s%d",buf,&x)==2)
	{
		string st=buf;
		relation2id[st]=x;
		id2relation[x]=st;
		relation_num++;
	}
    FILE* f_kb = fopen(("./"+dataset+"/test.txt").c_str(),"r");
    if(!f_kb)
    {
        printf(("can't open ./"+dataset+"/test.txt").c_str());
        exit(-1);
    }
	while (fscanf(f_kb,"%s",buf)==1)
    {
        string s1=buf;
        fscanf(f_kb,"%s",buf);
        string s2=buf;
        fscanf(f_kb,"%s",buf);
        string s3=buf;
        if (entity2id.count(s1)==0)
        {
            cout<<"miss entity:"<<s1<<endl;
        }
        if (entity2id.count(s2)==0)
        {
            cout<<"miss entity:"<<s2<<endl;
        }
        if (relation2id.count(s3)==0)
        {
        	cout<<"miss relation:"<<s3<<endl;
            relation2id[s3] = relation_num;
            relation_num++;
        }
        test.add(entity2id[s1],entity2id[s2],relation2id[s3],true);
    }
    fclose(f_kb);
    FILE* f_kb1 = fopen(("./"+dataset+"/train.txt").c_str(),"r");
    if(!f_kb1)
    {
        printf(("can't open ./"+dataset+"/train.txt").c_str());
        exit(-1);
    }

	while (fscanf(f_kb1,"%s",buf)==1)
    {
        string s1=buf;
        fscanf(f_kb1,"%s",buf);
        string s2=buf;
        fscanf(f_kb1,"%s",buf);
        string s3=buf;
        if (entity2id.count(s1)==0)
        {
            cout<<"miss entity:"<<s1<<endl;
        }
        if (entity2id.count(s2)==0)
        {
            cout<<"miss entity:"<<s2<<endl;
        }
        if (relation2id.count(s3)==0)
        {
            relation2id[s3] = relation_num;
            relation_num++;
        }

        entity2num[relation2id[s3]][entity2id[s1]]+=1;
        entity2num[relation2id[s3]][entity2id[s2]]+=1;
        e2num[entity2id[s1]]+=1;
        e2num[entity2id[s2]]+=1;
        test.add(entity2id[s1],entity2id[s2],relation2id[s3],false);
    }
    fclose(f_kb1);
    FILE* f_kb2 = fopen(("./"+dataset+"/valid.txt").c_str(),"r");
    if(!f_kb2)
    {
        printf(("can't open ./"+dataset+"/valid.txt").c_str());
        exit(-1);
    }

	while (fscanf(f_kb2,"%s",buf)==1)
    {
        string s1=buf;
        fscanf(f_kb2,"%s",buf);
        string s2=buf;
        fscanf(f_kb2,"%s",buf);
        string s3=buf;
        if (entity2id.count(s1)==0)
        {
            cout<<"miss entity:"<<s1<<endl;
        }
        if (entity2id.count(s2)==0)
        {
            cout<<"miss entity:"<<s2<<endl;
        }
        if (relation2id.count(s3)==0)
        {
            relation2id[s3] = relation_num;
            relation_num++;
        }
        test.add(entity2id[s1],entity2id[s2],relation2id[s3],false);
    }
    fclose(f_kb2);
}


int main(int argc,char**argv)
{
    double rate = 0.001;
    double margin = 1;
    int method = 0;
    int nepoch;
    string strategy;

    if (argc<2)
        return 0;
    else
    {
        version = argv[1];
        nthread = String_to_Int(argv[2]);
        dataset = argv[3];
        n=String_to_Int(argv[4]);
        margin=String_to_Double(argv[5]);
        nepoch=String_to_Int(argv[6]);
        resultpath=argv[7];
        rate= String_to_Double(argv[8]);
        strategy = argv[9];

        cout<<"strategy:"<<strategy<<" dataset:"<<dataset<<" nthread:"<<nthread<<" k:"<<n<<" margin:"<<margin<<" nepoch:"<<nepoch<<" rate:"<<rate<<" version:"<<version<<endl;

        time_t start,stop;
        start = time(NULL);
        prepare();
        stop = time(NULL);
        printf("Prepare Time:%ld\n",(stop-start));

        test.run();
        stop = time(NULL);
        printf("Total Time:%ld\n",(stop-start));

    }
    
}
