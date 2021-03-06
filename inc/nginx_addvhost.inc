function funct_nginxaddvhost {
PUREUSER=nginx
PUREGROUP=nginx
CNIP=$(ip route get 8.8.8.8 | awk 'NR==1 {print $NF}')
pureftpinstall

echo ""
cecho "---------------------------------------------" $boldyellow

read -ep "Enter vhost domain name you want to add (without www. prefix): " vhostname
if [[ "$PUREFTPD_DISABLED" = [nN] ]]; then
  if [ ! -f /usr/sbin/cracklib-check ]; then
    yum -y -q install cracklib
  fi
  if [ ! -f /usr/bin/pwgen ]; then
    yum -y -q install pwgen
  fi
  read -ep "Create FTP username for vhost domain (enter username): " ftpuser
  read -ep "Do you want to auto generate FTP password (recommended) [y/n]: " autogenpass

  if [[ "$autogenpass" = [yY] ]]; then
    ftppass=$(pwgen -1cnys 21)
  else
    read -ep "Create FTP password for $ftpuser (enter password): " ftppass
  
    # simple password strength check
    # utilise http://cracklib.sourceforge.net/ too
    CHECKPASSWD="$(cracklib-check <<<"$ftppass")"
    okay="$(awk -F': ' '{ print $2}' <<<"$CHECKPASSWD")"
    while [[ "$okay" != "OK" ]]; do
      echo "!! password strength not strong enough !! "
      echo "!! do not use common dictionary words !! "
      echo "!! do not use short passwords !! "
      echo "!! do not use simplistic passwords !! "
      echo
      read -ep "re-enter FTP password for $ftpuser (enter password): " ftppass
      CHECKPASSWD="$(cracklib-check <<<"$ftppass")"
      okay="$(awk -F': ' '{ print $2}' <<<"$CHECKPASSWD")"
    done
  fi # autogenpass
  echo
  echo "FTP username you entered: $ftpuser"
  if [[ "$autogenpass" = [yY] ]]; then
    echo "FTP password auto generated: $ftppass"
  else
    echo "FTP password you entered: $ftppass"    
  fi
fi

echo ""

if [ ! -d /home/nginx/domains/$vhostname ]; then

# Checking Permissions, making directories, example index.html
umask 027
mkdir -p /home/nginx/domains/$vhostname/{public,private,log,backup}

if [[ "$PUREFTPD_DISABLED" = [nN] ]]; then
  ( echo ${ftppass} ; echo ${ftppass} ) | pure-pw useradd $ftpuser -u $PUREUSER -g $PUREGROUP -d /home/nginx/domains/$vhostname
  pure-pw mkdb
fi

cat > "/home/nginx/domains/$vhostname/public/index.html" <<END
<html>
<head>
<title>$vhostname</title>
</head>
<body>
<p>Welcome to $vhostname. This index.html page can be removed.</p>

<p>Useful Centmin Mod info and links to bookmark.</p>

<ul>
  <li>Getting Started Guide - <a href="http://centminmod.com/getstarted.html" target="_blank">http://centminmod.com/getstarted.html</a></li>
  <li>Latest Centmin Mod version - <a href="http://centminmod.com" target="_blank">http://centminmod.com</a></li>
  <li>Centmin Mod FAQ - <a href="http://centminmod.com/faq.html" target="_blank">http://centminmod.com/faq.html</a></li>
  <li>Change Log - <a href="http://centminmod.com/changelog.html" target="_blank">http://centminmod.com/changelog.html</a></li>
  <li>Google+ Page latest news <a href="http://centminmod.com/gpage" target="_blank">http://centminmod.com/gpage</a></li>
  <li>Centmin Mod Community Forum <a href="https://community.centminmod.com/" target="_blank">https://community.centminmod.com/</a></li>
  <li>Centmin Mod Twitter <a href="https://twitter.com/centminmod" target="_blank">https://twitter.com/centminmod</a></li>
  <li>Centmin Mod Facebook Page <a href="https://www.facebook.com/centminmodcom" target="_blank">https://www.facebook.com/centminmodcom</a></li>
</ul>

<p><a href="https://www.digitalocean.com/?refcode=c1cb367108e8" target="_blank">Cheap VPS Hosting at Digitalocean</a></p>

</body>
</html>
END

    cp -R $CUR_DIR/htdocs/custom_errorpages/* /home/nginx/domains/$vhostname/public
umask 022
chown -R nginx:nginx "/home/nginx/domains/$vhostname"
find "/home/nginx/domains/$vhostname" -type d -exec chmod g+s {} \;

# Setting up Nginx mapping
cat > "/usr/local/nginx/conf/conf.d/$vhostname.conf" <<END
# Centmin Mod Getting Started Guide
# must read http://centminmod.com/getstarted.html

# redirect from non-www to www 
# uncomment, save file and restart Nginx to enable
# if unsure use return 302 before using return 301
#server {
#            listen   80;
#            server_name $vhostname;
#            return 301 \$scheme://www.${vhostname}\$request_uri;
#       }

server {
  server_name $vhostname www.$vhostname;

# ngx_pagespeed & ngx_pagespeed handler
#include /usr/local/nginx/conf/pagespeed.conf;
#include /usr/local/nginx/conf/pagespeedhandler.conf;
#include /usr/local/nginx/conf/pagespeedstatslog.conf;

  # limit_conn limit_per_ip 16;
  # ssi  on;

  access_log /home/nginx/domains/$vhostname/log/access.log combined buffer=256k flush=60m;
  error_log /home/nginx/domains/$vhostname/log/error.log;

  root /home/nginx/domains/$vhostname/public;

  location / {

# block common exploits, sql injections etc
#include /usr/local/nginx/conf/block.conf;

  # Enables directory listings when index file not found
  #autoindex  on;

  # Shows file listing times as local time
  #autoindex_localtime on;

  # Enable for vBulletin usage WITHOUT vbSEO installed
  # More example Nginx vhost configurations at
  # http://centminmod.com/nginx_configure.html
  #try_files		\$uri \$uri/ /index.php;

  }

  include /usr/local/nginx/conf/staticfiles.conf;
  include /usr/local/nginx/conf/php.conf;
  include /usr/local/nginx/conf/drop.conf;
  #include /usr/local/nginx/conf/errorpage.conf;
  include /usr/local/nginx/conf/vts_server.conf;
}
END

echo 
cecho "---------------------------------------------" $boldyellow
cmservice nginx reload
if [[ "$PUREFTPD_DISABLED" = [nN] ]]; then
  cmservice pure-ftpd restart
fi

echo 
if [[ "$PUREFTPD_DISABLED" = [nN] ]]; then
cecho "---------------------------------------------" $boldyellow
cecho "FTP hostname : $CNIP" $boldwhite
cecho "FTP port : 21" $boldwhite
cecho "FTP mode : FTP (explicit SSL)" $boldwhite
cecho "FTP Passive (PASV) : ensure is checked/enabled" $boldwhite
cecho "FTP username created for $vhostname : $ftpuser" $boldwhite
cecho "FTP password created for $vhostname : $ftppass" $boldwhite
fi
cecho "---------------------------------------------" $boldyellow
cecho "vhost for $vhostname created successfully" $boldwhite
cecho "vhost conf file for $vhostname created: /usr/local/nginx/conf/conf.d/$vhostname.conf" $boldwhite
cecho "upload files to /home/nginx/domains/$vhostname/public" $boldwhite
cecho "vhost log files directory is /home/nginx/domains/$vhostname/log" $boldwhite

echo
cecho "Current vhost listing at: /usr/local/nginx/conf/conf.d/" $boldwhite

ls -Alhrt /usr/local/nginx/conf/conf.d/ | awk '{ printf "%-4s%-4s%-8s%-6s %s\n", $6, $7, $8, $5, $9 }'

cecho "---------------------------------------------" $boldyellow

else

echo ""
cecho "---------------------------------------------" $boldyellow
cecho "vhost for $vhostname already exists" $boldwhite
cecho "/home/nginx/domains/$vhostname already exists" $boldwhite
cecho "---------------------------------------------" $boldyellow
echo ""

fi


}