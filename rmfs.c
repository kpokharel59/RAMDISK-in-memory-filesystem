
#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <string.h>
#include <fuse.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

# define SIZE 255


long filesize;
char tempFileName[SIZE];

// Directory node containg information about node
// may be a file or directory
typedef struct rmfs_dirNode 
{
	char     dirNode_name[SIZE];
	struct stat *dirNode_info;
	struct rmfs_dirNode  *parent;
	struct rmfs_dirNode  *child;
	struct rmfs_dirNode  *sibling;
	char   *data;
	char  node_type;  // 'D' for directory 'F' for file
}Node;

Node *root;

// return Node pointer if a node exists
Node* pathLookup(const char * path,int flag){

	//printf("Krishna Path Lookup with path =%s",path);
	if(strcmp(path,"/")==0){
		return root;
	}
	char *tokenizedPath;
	char previousToken[SIZE];
	char savePath[PATH_MAX];
	strcpy(savePath,path);
	tokenizedPath=strtok(savePath,"/");

	Node *rootptr= root;
	Node *temp=NULL;
	int found=0;
	while(tokenizedPath !=NULL){
		//printf("token %s\n",tokenizedPath );
		for(temp=rootptr->child; temp != NULL; temp=temp->sibling){
			//printf("Krishna Inside for loop name= %s\n",temp->dirNode_name);
			if(strcmp(temp->dirNode_name,tokenizedPath)==0){
				found=1;
				break;
			}
		}
		//printf("Krishna After for loop\n");
		strcpy(previousToken,tokenizedPath);
		tokenizedPath=strtok(NULL,"/");
		if(found==1 ){
			//printf("krishna get attt=r returned1\n");
			if(tokenizedPath == NULL)
				return temp;
		}
		else {
			if(flag==1){
				strcpy(tempFileName,previousToken);
				//printf("krishna get attt=r returned2\n");
				return rootptr;
			}
			return NULL;
		}
		//printf("node found to be same as token =%s\n",temp->dirNode_name );
		rootptr=temp;
		found=0;
	}
	//printf("krishna get attt=r returned3\n");
	return NULL;
}

static int rmfs_getattr(const char *path, struct stat *stbuf)
{
	//printf("\n Krishna getattr called path= %s \n",path);
	memset(stbuf, 0, sizeof(struct stat));
	//printf("krishna after mem set\n");
	if(strcmp(path,"/")==0){
		stbuf->st_mode=S_IFDIR | 0755;
		stbuf ->st_nlink=2;
	}
	else{
		//printf("krishna before lookup\n");
		Node* nodeLookedup = pathLookup(path,0);
		//printf("after lookup\n");
		if(nodeLookedup==NULL){
			//printf("Krshna empty node\n");
			return -ENOENT;
		}
		else {
			//printf("krishna getattr here inside\n");
			stbuf->st_mode=nodeLookedup->dirNode_info->st_mode;
			stbuf->st_nlink=nodeLookedup->dirNode_info->st_nlink;
			stbuf->st_size=nodeLookedup->dirNode_info->st_size;
		}
	}
	//printf("Before zero return at get attr \n");
	return 0;
}

static int rmfs_open(const char *path, struct fuse_file_info *fi){
	// if file found return o else ENOENT
	Node * nodeLookedup=pathLookup(path,0);
	if(nodeLookedup==NULL)
		return -ENOENT;
	/*else if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;*/
		
	return 0;
}

static int rmfs_mkdir(const char *path, mode_t mode){
	//printf("Krishna mkdir called \n");
	Node *temp= pathLookup(path,1);
	Node *newNode= (Node *)malloc(sizeof(Node));
	newNode->dirNode_info=(struct stat*)malloc(sizeof(struct stat));
	if(newNode ==NULL)
		return -ENOSPC;
	filesize=filesize -sizeof(Node) -sizeof(struct stat);
	//printf("Krishna filesize ========= %ld\n",filesize);
	if(filesize <0)
		return -ENOSPC;
	strcpy(newNode->dirNode_name,tempFileName);
	newNode->dirNode_info->st_mode=S_IFDIR | 0755;
	newNode->dirNode_info->st_nlink=2;
	newNode->parent=temp;
	newNode->sibling=NULL;
	newNode->dirNode_info->st_size=4096;
	newNode->node_type='D';

	if(temp->child==NULL){
		temp->child=newNode;
		temp->dirNode_info->st_nlink+=1;
	}else{
		Node *tempChild=temp->child;
		while(tempChild->sibling!=NULL){
			tempChild=tempChild->sibling;
		}
		tempChild->sibling=newNode;
		temp->dirNode_info->st_nlink+=1;
	}	
	return 0;
}

static int rmfs_opendir(const char *path, struct fuse_file_info *fi){
	//printf("opendir called \n");
	Node * nodeLookedup=pathLookup(path,0);
	if(nodeLookedup!=NULL)
		return 0;
	else
		return -ENOENT;
}


static int rmfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
						off_t offset, struct fuse_file_info *fi){
	//printf("readdir called \n");
	Node * nodeLookedup=pathLookup(path,1);
	if(nodeLookedup == NULL)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	Node * temp;
	for(temp=nodeLookedup->child;temp!=NULL;temp=temp->sibling){
		filler(buf,temp->dirNode_name,NULL,0);
	}
	return 0;
}


static int rmfs_read(const char *path, char *buf, size_t size, off_t offset,
					 struct fuse_file_info *fi){
	//printf("read called \n");
	Node * nodeLookedup=pathLookup(path,0);
	if(nodeLookedup == NULL)
		return -ENOENT;

	if(nodeLookedup->node_type=='D')
		return -EISDIR;

	size_t len;
	len = nodeLookedup->dirNode_info->st_size;
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, nodeLookedup->data + offset, size);
	} else
		size = 0;

	return size;
}

static int rmfs_write(const char *path, const char * buf, size_t size, off_t offset,
						struct fuse_file_info *fi){
	//printf("write called \n");
	Node* nodeLookedup = pathLookup(path,0);
	//check whether path valid or not
	if(nodeLookedup==NULL)
		return -ENOENT;

	// check wheteher its file or not
	if(nodeLookedup->node_type=='D')
		return -EISDIR;

	//check whether space is available or not
	if(filesize<size)
		return -ENOSPC;

	size_t len;
	len = nodeLookedup->dirNode_info->st_size;

	//writing for the first time
	if(size > 0 && len == 0){
		nodeLookedup->data=(char *)malloc(sizeof(char)*size);
		offset=0;
		memcpy(nodeLookedup->data+offset,buf,size);
		nodeLookedup->dirNode_info->st_size=offset+size;
		filesize=filesize-size;
		return size;
	}
	else if(size >0){
		if(offset > len)
			offset=len;
		nodeLookedup->data=(char *)realloc(nodeLookedup->data, sizeof(char)*(offset+size));
		memcpy(nodeLookedup->data + offset,buf,size);
		nodeLookedup -> dirNode_info ->st_size= offset+size;
		filesize=filesize-size;
		return size;
	}
	return 0;
}

static int rmfs_unlink(const char * path){
	//printf("unlink called \n");

	Node* nodeLookedup = pathLookup(path,0);

	if(nodeLookedup==NULL){
		return -ENOENT;
	}
	else{
		//do not remove if the given directory is not empty
		//Node * parent=nodeLookedup->parent;
			// if child node of parent
		if(nodeLookedup->parent->child==nodeLookedup){
			nodeLookedup->parent->child=nodeLookedup->sibling;
		}
		else{
			Node *findNode= nodeLookedup->parent->child;
			while(findNode != NULL && findNode->sibling!=nodeLookedup)
				findNode=findNode->sibling;
			findNode->sibling=nodeLookedup->sibling;
			}

		filesize=filesize+sizeof(struct stat)+sizeof(Node)+sizeof(nodeLookedup->dirNode_info->st_size);
		free(nodeLookedup->data);
		free(nodeLookedup->dirNode_info);
		free(nodeLookedup);	
		return 0;

	}
	return -ENOENT;
}

static int rmfs_create(const char * path, mode_t mode, struct fuse_file_info *fi){
	//printf("krishna create called \n");
	Node* nodeLookedup = pathLookup(path,1);
	//check whether path valid or not
	if(nodeLookedup==NULL)
		return -ENOENT;
	
	Node *newFile= (Node*)malloc(sizeof(Node));

	if(filesize<= 0|| newFile==NULL)
		return -ENOMEM;

	newFile->dirNode_info=(struct stat*)malloc(sizeof(struct stat));
	strcpy(newFile->dirNode_name,tempFileName);
	newFile->dirNode_info->st_mode=S_IFREG|0755;
	newFile->parent=nodeLookedup;
	newFile->child=NULL;
	newFile->sibling=NULL;
	newFile->data=NULL;
	newFile->dirNode_info->st_nlink=1;
	newFile->dirNode_info->st_size=0;
	newFile->node_type='F';

	if(nodeLookedup->child==NULL)
		nodeLookedup->child=newFile;
	else{
		Node * temp=nodeLookedup->child;
		while(temp->sibling!=NULL)
			temp=temp->sibling;
		temp->sibling=newFile;
	}
	filesize=filesize - sizeof(Node) -sizeof(struct stat);

	return 0;
}

static int rmfs_rmdir(const char *path){
	//printf("Krishna _rmdir called \n");
	Node* nodeLookedup = pathLookup(path,0);
	//printf("Krishna Name %s\n",nodeLookedup->dirNode_name);
	if(nodeLookedup==NULL){
		//printf("Krishna Error\n" );
		return -ENOENT;
	}
	else{
		//do not remove if the given directory is not empty
		if(nodeLookedup->child!= NULL)
			return -ENOTEMPTY;
		else {
			// if child node of parent
			if(nodeLookedup->parent->child==nodeLookedup){
				nodeLookedup->parent->child=nodeLookedup->sibling;
			}
			else{
				Node *findNode= nodeLookedup->parent->child;
				while(findNode->sibling!=nodeLookedup)
					findNode=findNode->sibling;
				findNode->sibling=nodeLookedup->sibling;
				}
			nodeLookedup->parent->dirNode_info->st_nlink-=1;
			free(nodeLookedup->dirNode_info);
			free(nodeLookedup);
			filesize=filesize+sizeof(struct stat)+sizeof(Node);
			return 0;
		}

	}
	return -ENOENT;
}

static int rmfs_utime(const char *path, struct utimbuf *ubuf)
{
        return 0;
}

static int rmfs_rename(const char *path, const char *newpath)
{
	Node *oldNode=pathLookup(path,1);
	Node *newNode=pathLookup(newpath,1);

 if(oldNode!=NULL && newNode !=NULL){
		if(newNode->node_type=='D'){
			rmfs_create(newpath,oldNode->dirNode_info->st_mode,NULL);
			newNode=pathLookup(newpath,1);
			newNode->dirNode_info->st_mode=oldNode->dirNode_info->st_mode;
			newNode->dirNode_info->st_nlink=oldNode->dirNode_info->st_nlink;
			newNode->dirNode_info->st_size=oldNode->dirNode_info->st_size;
			memset(newNode->data,0,newNode->dirNode_info->st_size);
			if(oldNode->dirNode_info->st_size >0){
				newNode->data=(char*)realloc(newNode->data,sizeof(char)*oldNode->dirNode_info->st_size);
				if(newNode->data==NULL)
					return -ENOSPC;
				strcpy(newNode->data,oldNode->data);
			}
			newNode->dirNode_info->st_size=oldNode->dirNode_info->st_size;
			rmfs_unlink(path);
			return 0;
		}
		else if(newNode->node_type=='F'){
			memset(oldNode->dirNode_name,0,SIZE);
			strcpy(oldNode->dirNode_name,newNode->dirNode_name);
			return 0;
		}
	}
	else{
		return -ENOENT;
	}
	return 0;
}

// map fuse structure to corresponding function implementation
static struct fuse_operations rmfs_oper = {
	.getattr	= rmfs_getattr,
	.readdir	= rmfs_readdir,
	.open		= rmfs_open,
	.read		= rmfs_read,
	.mkdir	    = rmfs_mkdir,
	.rmdir      = rmfs_rmdir,
	.write      = rmfs_write,
	.unlink  	= rmfs_unlink,
	.opendir    = rmfs_opendir,
	.create     = rmfs_create,
	.rename		=rmfs_rename,
	.utime		=rmfs_utime,
};

int main(int argc, char *argv[])
{
	if(argc != 3){
		printf("Invalid Number of arguments \nSynatx : ramdisk /path/to/dir <size>\n");
		return 0;
	}

	filesize =((long)atoi(argv[2])) *1024*1024;
	//printf ("fileseize ============%ld\n",filesize);
	argc--;

	//Initialize the root node as '/'
	root =(Node*)malloc(sizeof(Node));
	strcpy(root -> dirNode_name,"/");
	root -> parent =NULL;
	root -> child =NULL;
	root -> sibling =NULL;
	root -> node_type='D';
	root -> data =NULL;
	root -> dirNode_info =(struct stat*)malloc(sizeof(struct stat));
	// initialize stat structure
	// all parameters are not utilize
	root->dirNode_info->st_mode =S_IFDIR |0755;
	root->dirNode_info->st_nlink=2;
	//root ->dirNode_info->st_size=0;

	filesize= filesize - sizeof(Node) -sizeof(struct stat);

	return fuse_main(argc, argv, &rmfs_oper, NULL);
}
