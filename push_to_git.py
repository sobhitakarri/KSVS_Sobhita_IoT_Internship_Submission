import sys
import re
from dulwich.repo import Repo
from dulwich import porcelain

def push_repo(remote_url, branch="main", token=None, username="sobhitakarri"):
    repo = Repo('.')
    target_url = remote_url
    
    if token:
        if "@github.com" not in target_url:
            target_url = target_url.replace("https://", f"https://{username}:{token}@")
        password = token
    else:
        # Check if token is embedded in URL
        match = re.search(r"https://([^:]+):([^@]+)@github\.com", target_url)
        if match:
            username = match.group(1)
            password = match.group(2)
        else:
            match_token_only = re.search(r"https://([^@]+)@github\.com", target_url)
            if match_token_only:
                username = username or "sobhitakarri"
                password = match_token_only.group(1)
            else:
                password = None

    print(f"Pushing branch '{branch}' to GitHub...")
    try:
        porcelain.push(
            repo, 
            target_url, 
            refspecs=[f"refs/heads/{branch}:refs/heads/{branch}"], 
            password=password, 
            username=username
        )
        print(" Successfully pushed all code and documentation to your GitHub repository!")
    except Exception as e:
        print(f"Push error: {e}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: py push_to_git.py <REMOTE_GIT_URL> [GITHUB_TOKEN] [USERNAME]")
    else:
        url = sys.argv[1]
        token = sys.argv[2] if len(sys.argv) > 2 else None
        user = sys.argv[3] if len(sys.argv) > 3 else "sobhitakarri"
        push_repo(url, token=token, username=user)
