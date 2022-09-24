xcopy /Y ..\src\kernels\* %1\kernels\
xcopy /Y ..\src\shaders\* %1\shaders\
xcopy /Y ..\resources\* %1\resources\
xcopy /Y ..\assets\* %1\assets\

rem xcopy /Y ..\src\laplaceProcess.hpp %1\mush\include\
rem xcopy /Y ..\src\nullProcess.hpp %1\mush\include\
rem xcopy /Y ..\src\delayProcess.hpp %1\mush\include\
rem xcopy /Y ..\src\fixedExposureProcess.hpp %1\mush\include\
rem xcopy /Y ..\src\guiExposureProcess.hpp %1\mush\include\

rem xcopy /Y ..\src\integerMapBuffer.hpp %1\mush\include\
rem xcopy /Y ..\src\integerMapProcess.hpp %1\mush\include\


rem xcopy /Y ..\src\psnrProcess.hpp %1\mush\include\
rem xcopy /Y ..\src\x264Encoder.hpp %1\mush\include\
rem xcopy /Y ..\src\ffmpegWrapper.hpp %1\mush\include\
rem xcopy /Y ..\src\encoderEngine.hpp %1\mush\include\
rem xcopy /Y ..\src\switcherProcess.hpp %1\mush\include\
rem xcopy /Y ..\src\metricReporter.hpp %1\mush\include\
rem xcopy /Y ..\src\ffmpegEncodeDecode.hpp %1\mush\include\

rem xcopy /Y ..\src\tagInGui.hpp %1\mush\include\
rem xcopy /Y ..\src\ringBuffer.hpp %1\mush\include\
rem xcopy /Y ..\src\quitEventHandler.hpp %1\mush\include\
rem xcopy /Y ..\src\opencl.hpp %1\mush\include\
rem xcopy /Y ..\src\processNode.hpp %1\mush\include\
rem xcopy /Y ..\src\imageProperties.hpp %1\mush\include\
rem xcopy /Y ..\src\imageProcessor.hpp %1\mush\include\
rem xcopy /Y ..\src\imageProcess.hpp %1\mush\include\
rem xcopy /Y ..\src\imageBuffer.hpp %1\mush\include\
rem xcopy /Y ..\src\rotateImage.hpp %1\mush\include\
rem xcopy /Y ..\src\guiAccessible.hpp %1\mush\include\
rem xcopy /Y ..\src\exports.hpp %1\mush\include\
rem xcopy /Y ..\src\frameStepper.hpp %1\mush\include\
rem xcopy /Y ..\src\ConfigStruct.hpp %1\mush\include\
rem xcopy /Y ..\src\dll.hpp %1\mush\include\
rem xcopy /Y ..\src\CL\* %1\mush\include\CL\


rem xcopy /Y %1\libazure.lib %1\mush\lib\x64\Release\
rem xcopy /Y %1\libscarlet.lib %1\mush\lib\x64\Release\

rem xcopy /Y %1\video-mush.lib %1\mush\lib\x64\Release\
rem xcopy /Y %1\video-mush.pdb %1\mush\bin\x64\Release\
rem xcopy /Y %1\video-mush.dll %1\mush\bin\x64\Release\
rem xcopy /Y %1\libscarlet.dll %1\mush\bin\x64\Release\
rem xcopy /Y %1\libazure.dll %1\mush\bin\x64\Release\

rem xcopy /Y %1\assets\* %1\mush\assets\assets\
rem xcopy /Y %1\kernels\* %1\mush\assets\kernels\
rem xcopy /Y %1\shaders\* %1\mush\assets\shaders\
rem xcopy /Y %1\shaders\transferfunctions\* %1\mush\assets\shaders\transferfunctions\
rem xcopy /Y %1\resources\* %1\mush\assets\resources\
