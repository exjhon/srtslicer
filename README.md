# srtslicer
Use this C++ Console Program to slice your .wav files into clips by .srt timestamps and subtitles .

根据字幕文件，自动将wav音频切片，并且重命名为序号+字幕内容。
如果文件名中存在? : 这样的非法字符则会自动删掉。
下载release里的srtslicer.exe即可使用。需要安装ffmpeg。

最新的更新会重命名为原wav文件名_序号_起始时间_结束时间_内容。
