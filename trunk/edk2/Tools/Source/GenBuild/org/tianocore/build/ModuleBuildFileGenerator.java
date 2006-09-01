/** @file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 **/
package org.tianocore.build;

import java.io.File;
import java.util.LinkedHashMap;
import java.util.Map;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.tianocore.build.fpd.FpdParserTask;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.global.PropertyManager;
import org.tianocore.build.id.FpdModuleIdentification;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.build.id.PackageIdentification;
import org.w3c.dom.Comment;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;

public class ModuleBuildFileGenerator {

    ///
    /// Pass: TARGET, TOOLCHAIN, ARCH
    /// PACKAGE, PACKAGE_GUID, PACKAGE_VERSION
    ///
    String[] inheritProperties = {"ARCH", "MODULE_GUID", "MODULE_VERSION", "PLATFORM_FILE", "PACKAGE_GUID", "PACKAGE_VERSION"};

    ///
    /// The information at the header of <em>build.xml</em>.
    ///
    private String info = "DO NOT EDIT \n"
                        + "This file is auto-generated by the build utility\n"
                        + "\n"
                        + "Abstract:\n"
                        + "Auto-generated ANT build file for build EFI Modules and Platforms\n";

    private FpdModuleIdentification fpdModuleId;
    
    private Project project;
    
    private String ffsKeyword;
    
    private String[] includes;
    
    private SurfaceAreaQuery saq = null;
    
    public ModuleBuildFileGenerator(Project project, String ffsKeyword, FpdModuleIdentification fpdModuleId, String[] includes, SurfaceAreaQuery saq) {
        this.project = project;
        this.fpdModuleId = fpdModuleId;
        this.ffsKeyword = ffsKeyword;
        this.includes = includes;
        this.saq = saq;
    }
    
    /**
      The whole BaseName_build.xml is composed of seven part. 
      <ul>
      <li> ANT properties; </li>
      <li> Dependent module (dependent library instances in most case); </li>
      <li> Source files; </li>
      <li> Sections if module is not library; </li>
      <li> Output (different for library module and driver module); </li>
      <li> Clean; </li>
      <li> Clean all. </li>
      </ul>
      
      @throws BuildException
              Error throws during BaseName_build.xml generating. 
    **/
    public void genBuildFile(String buildFilename) throws BuildException {
        FfsProcess fp = new FfsProcess(saq);
        DocumentBuilderFactory domfac = DocumentBuilderFactory.newInstance();
        try {
            DocumentBuilder dombuilder = domfac.newDocumentBuilder();
            Document document = dombuilder.newDocument();
            Comment rootComment = document.createComment(info);
            
            //
            // create root element and its attributes
            //
            Element root = document.createElement("project");
            root.setAttribute("name", fpdModuleId.getModule().getName());
            root.setAttribute("default", "all");
            root.setAttribute("basedir", ".");
            
            //
            // element for External ANT tasks
            //
            root.appendChild(document.createComment("Apply external ANT tasks"));
            Element ele = document.createElement("taskdef");
            ele.setAttribute("resource", "frameworktasks.tasks");
            root.appendChild(ele);
            ele = document.createElement("taskdef");
            ele.setAttribute("resource", "cpptasks.tasks");
            root.appendChild(ele);
            ele = document.createElement("typedef");
            ele.setAttribute("resource", "cpptasks.types");
            root.appendChild(ele);
            ele = document.createElement("taskdef");
            ele.setAttribute("resource", "net/sf/antcontrib/antlib.xml");
            root.appendChild(ele);

            //
            // Generate the default target,
            // which depends on init, sections and output target
            //
            root.appendChild(document.createComment("Default target"));
            ele = document.createElement("target");
            ele.setAttribute("name", "all");
            ele.setAttribute("depends", "libraries, sourcefiles, sections, output");
            root.appendChild(ele);
            
            //
            // compile all source files
            //
            root.appendChild(document.createComment("Compile all dependency Library instances."));
            ele = document.createElement("target");
            ele.setAttribute("name", "libraries");

            //
            // Parse all sourfiles but files specified in sections
            //
            applyLibraryInstance(document, ele);
            root.appendChild(ele);

            //
            // compile all source files
            //
            root.appendChild(document.createComment("sourcefiles target"));
            ele = document.createElement("target");
            ele.setAttribute("name", "sourcefiles");
            
            //
            // Parse all sourfiles but files specified in sections
            //
            applyCompileElement(document, ele);
            root.appendChild(ele);

            //
            // generate the init target
            // main purpose is create all nessary pathes
            // generate the sections target
            //
            root.appendChild(document.createComment("sections target"));
            ele = document.createElement("target");
            ele.setAttribute("name", "sections");
            applySectionsElement(document, ele, fp);
            root.appendChild(ele);

            //
            // generate the output target
            //
            root.appendChild(document.createComment("output target"));
            ele = document.createElement("target");
            ele.setAttribute("name", "output");
            applyOutputElement(document, ele, fp);
            root.appendChild(ele);

            
            //
            // generate the clean target
            //
            root.appendChild(document.createComment("clean target"));
            ele = document.createElement("target");
            ele.setAttribute("name", "clean");
            applyCleanElement(document, ele);
            root.appendChild(ele);
            
            //
            // generate the Clean All target
            //
            root.appendChild(document.createComment("Clean All target"));
            ele = document.createElement("target");
            ele.setAttribute("name", "cleanall");
            applyDeepcleanElement(document, ele);
            root.appendChild(ele);
            
            //
            // add the root element to the document
            //
            document.appendChild(rootComment);
            document.appendChild(root);
            //
            // Prepare the DOM document for writing
            //
            Source source = new DOMSource(document);

            //
            // Prepare the output file
            //
            File file = new File(buildFilename);

            //
            // generate all directory path
            //
            (new File(file.getParent())).mkdirs();
            FileOutputStream outputStream = new FileOutputStream(file);
            Result result = new StreamResult(new OutputStreamWriter(outputStream));
            
            //
            // Write the DOM document to the file
            //
            Transformer xformer = TransformerFactory.newInstance().newTransformer();
            xformer.setOutputProperty("{http://xml.apache.org/xslt}indent-amount", "2");
            xformer.setOutputProperty(OutputKeys.INDENT, "yes");
            xformer.transform(source, result);
        } catch (Exception ex) {
            throw new BuildException("Generating the module [" + fpdModuleId.getModule().getName() + "] build.xml file failed!.\n" + ex.getMessage());
        }
    }

    /**
      Generate the clean elements for BaseName_build.xml. 
      
      @param document current BaseName_build.xml XML document
      @param root Root element for current
    **/
    private void applyCleanElement(Document document, Node root) {
        //
        // <delete includeemptydirs="true">
        //   <fileset dir="${DEST_DIR_OUTPUT}" includes="" excludes="" />
        // </delete>
        //
        Element deleteEle = document.createElement("delete");
        deleteEle.setAttribute("includeemptydirs", "true");
        Element filesetEle = document.createElement("fileset");
        filesetEle.setAttribute("dir", "${DEST_DIR_OUTPUT}");
        filesetEle.setAttribute("includes", "**/*");
        filesetEle.setAttribute("excludes", "*.xml");
        deleteEle.appendChild(filesetEle);
        root.appendChild(deleteEle);
    }

    /**
      Generate the cleanall elements for BaseName_build.xml. 
      
      @param document current BaseName_build.xml XML document
      @param root Root element for current
    **/
    private void applyDeepcleanElement(Document document, Node root) {
        //
        // <delete includeemptydirs="true">
        //   <fileset dir="${DEST_DIR_OUTPUT}" includes="" excludes="" />
        // </delete>
        //
        Element deleteEle = document.createElement("delete");
        deleteEle.setAttribute("includeemptydirs", "true");
        Element filesetEle = document.createElement("fileset");
        filesetEle.setAttribute("dir", "${DEST_DIR_OUTPUT}");
        filesetEle.setAttribute("includes", "**/*");
        filesetEle.setAttribute("excludes", "*.xml");
        deleteEle.appendChild(filesetEle);
        root.appendChild(deleteEle);
        
        //
        // <delete includeemptydirs="true">
        //   <fileset dir="${DEST_DIR_DEBUG}" includes="" />
        // </delete>
        //
        deleteEle = document.createElement("delete");
        deleteEle.setAttribute("includeemptydirs", "true");
        filesetEle = document.createElement("fileset");
        filesetEle.setAttribute("dir", "${DEST_DIR_DEBUG}");
        filesetEle.setAttribute("includes", "**/*");
        deleteEle.appendChild(filesetEle);
        root.appendChild(deleteEle);
    }

    /**
      Generate the dependent library instances elements for BaseName_build.xml. 
      
      @param document current BaseName_build.xml XML document
      @param root Root element for current
    **/
    private void applyLibraryInstance(Document document, Node root) {
        ModuleIdentification[] libinstances = saq.getLibraryInstance(fpdModuleId.getArch());
        for (int i = 0; i < libinstances.length; i++) {
            //
            // Put package file path to module identification
            //
            PackageIdentification packageId = libinstances[i].getPackage();
            
            //
            // Generate ANT script to build library instances
            //
            Element ele = document.createElement("GenBuild");
            ele.setAttribute("type", "build");
            
            //
            // Prepare pass down information
            //
            Map<String, String> passDownMap = new LinkedHashMap<String, String>();
            for (int j = 0; j < inheritProperties.length; j ++){
                passDownMap.put(inheritProperties[j], "${" + inheritProperties[j] + "}");
            }
            
            passDownMap.put("MODULE_GUID", libinstances[i].getGuid());
            passDownMap.put("MODULE_VERSION", libinstances[i].getVersion());
            
            passDownMap.put("PACKAGE_GUID", packageId.getGuid());
            passDownMap.put("PACKAGE_VERSION", packageId.getVersion());
            
            for (int j = 0; j < inheritProperties.length; j ++){
                Element property = document.createElement("property");
                property.setAttribute("name", inheritProperties[j]);
                property.setAttribute("value", passDownMap.get(inheritProperties[j]));
                ele.appendChild(property);
            }
            
            root.appendChild(ele);
        }
    }

    /**
      Generate the build source files elements for BaseName_build.xml. 
      
      @param document current BaseName_build.xml XML document
      @param root Root element for current
    **/
    private void applyCompileElement(Document document, Node root) {
        //
        // sourceFiles[][0] is FileType, [][1] is File name relative to Module_Dir
        //
        String[][] sourceFiles = saq.getSourceFiles(fpdModuleId.getArch());

        FileProcess fileProcess = new FileProcess();
        fileProcess.init(project, includes, document);
        
        //
        // Initialize some properties by user
        //
        Element initEle = document.createElement("Build_Init");
        root.appendChild(initEle);

        String moduleDir = project.getProperty("MODULE_DIR");
        //
        // Parse all Unicode files
        //
        for (int i = 0; i < sourceFiles.length; i++) {
            //
            // Go through all source files. Add MODULE_DIR to preffix
            //
            File sourceFile =  new File(moduleDir + File.separatorChar + sourceFiles[i][1]);
            sourceFiles[i][1] = sourceFile.getPath();
            String filetype = sourceFiles[i][0];
            if (filetype != null) {
                fileProcess.parseFile(sourceFiles[i][1], filetype, root, true);
            } else {
                fileProcess.parseFile(sourceFiles[i][1], root, true);
            }
        }
        
        //
        // If exist Unicode file
        //
        if (fileProcess.isUnicodeExist()) {
            Element ele = document.createElement("Build_Unicode_Database");
            ele.setAttribute("FILEPATH", ".");
            ele.setAttribute("FILENAME", "${BASE_NAME}");
            Element includesEle = document.createElement("EXTRA.INC");
            for (int i = 0; i < includes.length; i++) {
                Element includeEle = document.createElement("includepath");
                includeEle.setAttribute("path", includes[i]);
                includesEle.appendChild(includeEle);
            }
            ele.appendChild(includesEle);
            root.appendChild(ele);
        }

        //
        // Parse AutoGen.c & AutoGen.h
        //
        if ( ! fpdModuleId.getModule().getName().equalsIgnoreCase("Shell")) {
            fileProcess.parseFile(project.getProperty("DEST_DIR_DEBUG") + File.separatorChar + "AutoGen.c", root, false);
        }
        
        //
        // Parse all source files but Unicode files
        //
        for (int i = 0; i < sourceFiles.length; i++) {
            String filetype = sourceFiles[i][0];
            if (filetype != null) {
                fileProcess.parseFile(sourceFiles[i][1], filetype, root, false);
            } else {
                fileProcess.parseFile(sourceFiles[i][1], root, false);
            }
        }
        
        //
        // Initialize SOURCE_FILES for dependcy check use
        //
        String str = "";
        for (int i = 0; i < sourceFiles.length; i++) {
            str += " " + sourceFiles[i][1];
        }
        PropertyManager.setProperty(project, "SOURCE_FILES", str.replaceAll("(\\\\)", "/"));
    }

    /**
      Generate the section elements for BaseName_build.xml. Library module will
      skip this process.  
      
      @param document current BaseName_build.xml XML document
      @param root Root element for current
    **/
    private void applySectionsElement(Document document, Node root, FfsProcess fp) {
        if (fpdModuleId.getModule().isLibrary()) {
            return ;
        }
        if (fp.initSections(ffsKeyword, project, fpdModuleId)) {
            String targetFilename = fpdModuleId.getModule().getGuid() + "-" + "${BASE_NAME}" + FpdParserTask.getSuffix(fpdModuleId.getModule().getModuleType());
            String[] list = fp.getGenSectionElements(document, "${BASE_NAME}", fpdModuleId.getModule().getGuid(), targetFilename);

            for (int i = 0; i < list.length; i++) {
                Element ele = document.createElement(list[i]);
                ele.setAttribute("FILEPATH", ".");
                ele.setAttribute("FILENAME", "${BASE_NAME}");
                root.appendChild(ele);
            }
        }
    }

    /**
      Generate the output elements for BaseName_build.xml. If module is library,
      call the <em>LIB</em> command, else call the <em>GenFfs</em> command. 
      
      @param document current BaseName_build.xml XML document
      @param root Root element for current
    **/
    private void applyOutputElement(Document document, Node root, FfsProcess fp) {
        if (fpdModuleId.getModule().isLibrary()) {
            //
            // call Lib command
            //
            Element cc = document.createElement("Build_Library");
            cc.setAttribute("FILENAME", fpdModuleId.getModule().getName());
            root.appendChild(cc);
        }
        //
        // if it is a module but library
        //
        else {
            if (fp.getFfsNode() != null) {
                root.appendChild(fp.getFfsNode());
            }
        }
    }

}
